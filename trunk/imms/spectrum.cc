#include <math.h>

#include <iostream>
#include <utility>

#include "spectrum.h"

using std::endl;
using std::cerr;
using std::pair;

static int bounds[] =
    { -1, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255 };

static int scales[] =
    { 11000, 4100, 3600, 3000, 2400, 2000, 1700, 2000, 1500, 1000, 750,
        650, 550, 450, 200, 80 };

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

string spectrum_normalize(const string &spectr)
{
    pair<float, float> stats = spectrum_analyze(spectr);

    string normal = "";

    // rescale the amplitude and shift to match means
    float divscale = 7 / stats.second;
    for (int i = 0; i < short_spectrum_size; ++i)
        normal += ROUND('m' + (spectr[i] - stats.first) * divscale);

    return normal;
}

int spectrum_distance(const string &s1, const string &s2)
{
    if (s1.length() != s2.length()
            && (int)s1.length() != short_spectrum_size)
        return -1;

    string normal_s1 = spectrum_normalize(s1);
    string normal_s2 = spectrum_normalize(s2);

    int distance = 0;

    for (int i = 0; i < short_spectrum_size; ++i)
        distance += ROUND(pow(normal_s1[i] - normal_s2[i], 2));

    return distance;
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
        char c = 'a' + ROUND(('z' - 'a') * average / scales[i]);
        if (c > 'z')
            c = 'z';

        short_spectrum += c;
    }

#ifdef DEBUG
    cerr << "spectrum [" << short_spectrum << "]" << endl;
#endif

    if (have_spectrums > 100)
        immsdb.set_spectrum(short_spectrum);

    int distance = spectrum_distance(last_spectrum, short_spectrum);
#ifdef DEBUG
    cerr << "difference from last: " << distance << endl;
#endif

    last_spectrum = short_spectrum;
}
