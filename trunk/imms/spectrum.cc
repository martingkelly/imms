#include <math.h>
#include <assert.h>

#include <iostream>
#include <utility>

#include "spectrum.h"

using std::endl;
using std::cerr;
using std::pair;

static int bounds[] =
    { -1, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255 };

static int scales[] =
    { 9200, 4900, 4200, 3500, 2600, 2100, 1650, 1200, 1000, 780, 550,
        400, 290, 200, 110, 41 };

SpectrumAnalyzer::SpectrumAnalyzer()
{
    last_spectrum = "";
    reset();
}

void SpectrumAnalyzer::reset()
{
    have_spectrums = 0;
    memset(long_spectrum, 0, sizeof(long_spectrum));
}

pair<float, float> spectrum_analyze(const string &spectstr)
{
    float mean = 0;
    for (int i = 0; i < short_spectrum_size; ++i)
        mean += spectstr[i];

    mean /= short_spectrum_size;

    float deviation = 0;
    for (int i = 0; i < short_spectrum_size; ++i)
        deviation += pow(mean - spectstr[i], 2);

    deviation = sqrt(deviation / short_spectrum_size);

    return pair<float, float>(mean, deviation);
}

int spectrum_power(const string &spectstr)
{
    float mean = 0;
    for (int i = 0; i < short_spectrum_size; ++i)
        mean += spectstr[i] *
           pow(1 + (short_spectrum_size - i) / 16, 5);

    return ROUND(mean / short_spectrum_size);
}

string spectrum_normalize(const string &spectr)
{
    pair<float, float> stats = spectrum_analyze(spectr);

    string normal = "";

    // rescale the amplitude and shift to match means
    float divscale = 7 / stats.second;
    for (int i = 0; i < short_spectrum_size; ++i)
    {
         int val = ROUND('m' + (spectr[i] - stats.first) * divscale);
         val = val < 0 ? 0 : val;
         val = val > 127 ? 127 : val;
         normal += (char)val;
    }

    return normal;
}

int spectrum_distance(const string &s1, const string &s2)
{
    int distance = 0;

    for (int i = 0; i < short_spectrum_size; ++i)
        distance += ROUND(pow(s1[i] - s2[i], 2));

    return distance;
}

float SpectrumAnalyzer::evaluate_transition(const string &from,
        const string &to)
{
    if (from.length() != to.length()
            && (int)from.length() != short_spectrum_size)
        assert(0 && "malformed spectrums in evaluate_transition");

    string normal_from = spectrum_normalize(from);
    string normal_to = spectrum_normalize(to);

    float distance = spectrum_distance(normal_from, normal_to);
    float dscore = pow(1 - distance / 1000, 2);
    dscore = distance < 0 ? -dscore : dscore;

    cerr << "distance = " << distance << " dscore = " << dscore << endl;

    return dscore;
}

void SpectrumAnalyzer::integrate_spectrum(
        uint16_t _long_spectrum[long_spectrum_size])
{
    static char delay = 0;
    if (++delay % 32 != 0)
        return;

    for (int i = 0; i < long_spectrum_size; ++i)
        long_spectrum[i] = (long_spectrum[i] * have_spectrums
                + _long_spectrum[i]) / (have_spectrums + 1);

    ++have_spectrums;
}

void SpectrumAnalyzer::finalize()
{
    if (!have_spectrums)
        return;

    string short_spectrum = "";

    for (int i = 0; i < short_spectrum_size; ++i)
    {
        double average = 0;
        for (int j = bounds[i] + 1; j <= bounds[i + 1]; ++j)
            average += long_spectrum[j];

        average /= (bounds[i + 1] - bounds[i]);
        int c = 'a' + ROUND(('z' - 'a') * average / scales[i]);
        c = c > 'z' ? 'z' : c;
        c = c < 'a' ? 'a' : c;

        short_spectrum += (char)c;
    }

#ifdef DEBUG
    cerr << "spectrum [" << short_spectrum << "]" << endl;
    cerr << "power rating = " << spectrum_power(short_spectrum) << endl;
#endif

    if (have_spectrums > 100)
        immsdb.set_spectrum(short_spectrum);

    last_spectrum = short_spectrum;
}
