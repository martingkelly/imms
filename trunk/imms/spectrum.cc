#include <iostream>
#include <math.h>

#include "spectrum.h"

using std::endl;
using std::cerr;

static int bounds[] =
    { -1, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255 };

static int scales[] =
    { 11000, 4100, 3600, 3000, 2400, 2000, 1700, 2000, 1500, 1000, 750,
        650, 550, 450, 200, 80 };

// BeatKeeper
BeatKeeper::BeatKeeper()
{
    beats = max_bass = average_bass = 0;
}

inline int weighted_merge(int v1, int v2, double v2_weight)
{ return ROUND(pow(sqrt(v1) * (1 - v2_weight) + sqrt(v2) * v2_weight, 2)); }

void BeatKeeper::integrate_bass(int bass)
{
    if (bass > average_bass + (max_bass - average_bass) * 0.8)
    {
        if (!above_average)
            ++beats;
        above_average = true;
    }

    if (bass < average_bass)
        above_average = false;

    if (bass > max_bass)
        max_bass = bass;

    max_bass = weighted_merge(max_bass, bass, 0.01);
    average_bass = weighted_merge(average_bass, bass, 0.05);
}

int BeatKeeper::get_BPM(time_t seconds)
{
    int bpm = ROUND(beats * 60.0 / seconds);
    beats = max_bass = average_bass = 0;
    above_average = false;
    return bpm;
}

SpectrumAnalyzer::SpectrumAnalyzer()
{
    last_spectrum = "";
}

void SpectrumAnalyzer::reset()
{
    memset(long_spectrum, 0, sizeof(long_spectrum));
}

void SpectrumAnalyzer::start_spectrum_analysis()
{
    spectrum_start_time = time(0);
    have_spectrums = 0;
}

void SpectrumAnalyzer::stop_spectrum_analysis()
{
    time_t seconds = time(0) - spectrum_start_time;

    int bpmv0 = bpm0.get_BPM(seconds);
    int bpmv1 = bpm1.get_BPM(seconds);

    int min = bpmv0;
    if (bpmv1 < min)
        min = bpmv1;

    bpm = min;

    cerr << "BPM: " << min << " (" << bpmv0 << "/" << bpmv1 << ")" << endl;
}

void SpectrumAnalyzer::integrate_spectrum(
        uint16_t _long_spectrum[long_spectrum_size])
{
    static char delay = 0;
    if (++delay % 32 == 0)
    {
        for (int i = 0; i < long_spectrum_size; ++i)
            long_spectrum[i] = (long_spectrum[i] * have_spectrums
                    + _long_spectrum[i]) / (have_spectrums + 1);

        ++have_spectrums;
    }

    bpm0.integrate_bass(1000 * _long_spectrum[0] / scales[0]);
    bpm1.integrate_bass(1000 * _long_spectrum[1] / scales[1]);
}

void SpectrumAnalyzer::finalize_spectrum()
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

    int distance = 0;

    if (last_spectrum.length() == short_spectrum.length()
            && (int)last_spectrum.length() == short_spectrum_size)
        for (int i = 0; i < short_spectrum_size; ++i)
            distance += ROUND(pow(short_spectrum[i] - last_spectrum[i], 2));

    last_spectrum = short_spectrum;
#ifdef DEBUG
    cerr << "difference from last: " << distance << endl;
#endif
}
