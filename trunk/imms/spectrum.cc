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

static float scales[] =
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
    memset(spectrum, 0, sizeof(spectrum));
}

pair<float, float> spectrum_analyze(const string &spectstr)
{
    float mean = 0;
    for (int i = 0; i < SHORTSPECTRUM; ++i)
        mean += spectstr[i];

    mean /= SHORTSPECTRUM;

    float deviation = 0;
    for (int i = 0; i < SHORTSPECTRUM; ++i)
        deviation += pow(mean - spectstr[i], 2);

    deviation = sqrt(deviation / SHORTSPECTRUM);

    return pair<float, float>(mean, deviation);
}

int spectrum_power(const string &spectstr)
{
    float mean = 0;
    for (int i = 0; i < SHORTSPECTRUM; ++i)
        mean += (spectstr[i] - 'a') *
           pow(1 + (SHORTSPECTRUM - i) / 16, 4);

    return ROUND(mean / SHORTSPECTRUM);
}

string spectrum_normalize(const string &spectr)
{
    pair<float, float> stats = spectrum_analyze(spectr);

    string normal = "";

    // rescale the amplitude and shift to match means
    float divscale = 7 / stats.second;
    for (int i = 0; i < SHORTSPECTRUM; ++i)
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

    for (int i = 0; i < SHORTSPECTRUM; ++i)
        distance += ROUND(pow(s1[i] - s2[i], 2));

    return distance;
}

float SpectrumAnalyzer::evaluate_transition(const string &from,
        const string &to)
{
    if (from.length() != to.length()
            && (int)from.length() != SHORTSPECTRUM)
        assert(0 && "malformed spectrums in evaluate_transition");

    float distance =  1 - spectrum_distance(spectrum_normalize(from),
            spectrum_normalize(to)) / 1000.0;
    distance = distance < -1 ? -1 : distance;
    cerr << "distance = " << distance << endl;

    return distance;
}

void SpectrumAnalyzer::integrate_spectrum(
        uint16_t long_spectrum[LONGSPECTRUM])
{
    float power = 0;
    for (int i = 0; i < 3; ++i)
        power += long_spectrum[i] / (float)scales[i];

    bpm.integrate_beat(power);

    static char delay = 0;
    if (++delay % 32 != 0)
        return;

    for (int i = 0; i < SHORTSPECTRUM; ++i)
    {
        float average = 0;
        for (int j = bounds[i] + 1; j <= bounds[i + 1]; ++j)
            average += long_spectrum[j];

        average /= (bounds[i + 1] - bounds[i]);

        spectrum[i] = (spectrum[i] * have_spectrums + average)
            / (float)++have_spectrums;
    }
}

void SpectrumAnalyzer::finalize()
{
    if (!have_spectrums)
        return;

    bpm.getBPM();

    last_spectrum = "";

    for (int i = 0; i < SHORTSPECTRUM; ++i)
    {
        int c = 'a' + ROUND(('z' - 'a') * spectrum[i] / scales[i]);
        c = c > 'z' ? 'z' : (c < 'a' ? 'a' : c);

        last_spectrum += (char)c;
    }

#ifdef DEBUG
    cerr << "spectrum [" << last_spectrum << "]" << endl;
    cerr << "power rating = " << spectrum_power(last_spectrum) << endl;
#endif

    if (have_spectrums > 100)
        immsdb.set_spectrum(last_spectrum);
}
