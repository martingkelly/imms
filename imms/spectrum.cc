#include <math.h>
#include <assert.h>

#include <iostream>
#include <utility>
#include <fstream>
#include <map>

#include "spectrum.h"

using std::endl;
using std::cerr;
using std::pair;
using std::map;

void BeatKeeper::reset()
{
    samples = 0;
    first = last = 0;
    memset(data, 0, sizeof(data));
    memset(&prev, 0, sizeof(prev));
    memset(beats, 0, sizeof(beats));
    current_position = current_window = data;
    last_window = &data[MAXBEATLENGTH];
}

inline int offset2bpm(int offset)
{
    return ROUND(60 * SAMPLES/ (float)(MINBEATLENGTH + offset));
}

int BeatKeeper::getBPM()
{
    cerr << samples << " samples in " << last - first << " seconds: "
        << ROUND(samples / (float)(last - first)) << " samples/sec" << endl; 

    std::ofstream bstats("/tmp/beats", std::ios::trunc);

    float max = 0, min = INT_MAX;
    for (int i = 0; i < MAXBEATLENGTH - MINBEATLENGTH; ++i)
    {
        bstats << offset2bpm(i) << " " << ROUND(beats[i]) << endl;

        if (beats[i] > max)
            max = beats[i];
        if (beats[i] < min)
            min = beats[i];
    }

    // Find peaks in the beat distribution
    map<int, int> peaks;
    int up = 0, down = 0, last_peak = 0;
    for (int i = 1; i < MAXBEATLENGTH - MINBEATLENGTH; ++i)
    {
        if (beats[i] > beats[i - 1])
        {
            if (up++)
            {
                if (last_peak)
                    peaks[last_peak] = (peaks[last_peak] + down) / 2;
                last_peak = 0;
                down = 0;
            }
        }
        else if (beats[i] < beats[i - 1])
        {
            if (down++)
            {
                if (up > 2 && beats[i] > min + (max - min) * 0.75)
                {
                    last_peak = offset2bpm(i - 2);
                    peaks[last_peak] = up;
                    cerr << "peak at " << last_peak << endl;
                }
                up = 0;
            }
        }

        cerr << "i = " << offset2bpm(i) << " up = " << up 
            << " down = " << down << endl;
    }

    reset();

    if (peaks.empty())
    {
        cerr << "peak are empty" << endl;
        return -1;
    }

    if (peaks.size() == 1)
        return peaks.begin()->first;

    // coalesce peaks
    map<int, int> combined_peaks;
    for (map<int, int>::iterator i = peaks.begin(); i != peaks.end(); ++i)
    {
        map<int, int>::iterator dbl = peaks.lower_bound(i->first * 2 - 3);
        if (abs(dbl->first - i->first * 2) < 3)
        {
            cerr << "coalescing " << i->first 
                << " and " << dbl->first << endl; 
            dbl->second += i->second;
        }
        else
            combined_peaks[i->second] = i->first;
    }

    if (combined_peaks.size() == 1)
        return combined_peaks.begin()->second;

    map<int, int>::iterator i = combined_peaks.begin();
    int biggest = i->second;
    if (biggest * 2 / 3 > (++i)->second)
        return combined_peaks[biggest];

    return -1;
}

time_t usec_diff(struct timeval &tv1, struct timeval &tv2)
{
    return (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec;
}

const struct timeval& operator +=(struct timeval &tv, int usecs)
{
    tv.tv_usec += usecs;
    tv.tv_sec += tv.tv_usec / MICRO;
    tv.tv_usec %= MICRO;
    return tv;
}

void BeatKeeper::integrate_beat(float power)
{
    struct timeval now;
    gettimeofday(&now, 0);

    last = now.tv_sec;
    if (!first)
    {
        first = last;
        prev = now;
    }

    // find out how many timeslices we should count this sample for
    time_t diff = usec_diff(prev, now);
    int count_for = ROUND(diff * SAMPLES / (float)MICRO) % 10;

    samples += count_for;
    prev += count_for * MICRO / SAMPLES;

    for (int i = 0; i < count_for; ++i)
    {
        *current_position++ = power;
        if (current_position - current_window == MAXBEATLENGTH)
            process_window();
    }
}

void BeatKeeper::process_window()
{
    // update beat values
    for (int i = 0; i < MAXBEATLENGTH; ++i)
    {
        for (int offset = MINBEATLENGTH; offset < MAXBEATLENGTH; ++offset)
        {
            int p = i + offset;
            float warped = *(p < MAXBEATLENGTH ?
                    last_window + p : current_window + p - MAXBEATLENGTH);
            beats[offset - MINBEATLENGTH] += last_window[i] * warped;
        }
    }

    // swap the windows
    float *tmp = current_window;
    current_window = current_position = last_window;
    last_window = tmp;
}

static int bounds[] =
    { -1, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255 };

static float scales[] =
    { 9800, 5000, 4200, 3500, 2600, 2100, 1650, 1200, 1000, 780, 550,
        400, 290, 200, 110, 42 };

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
    cerr << "BPM = " << bpm.getBPM() << endl;

    if (!have_spectrums)
        return;

    last_spectrum = "";

    for (int i = 0; i < SHORTSPECTRUM; ++i)
    {
        int c = 'a' + ROUND(('z' - 'a') * spectrum[i] / scales[i]);
        c = c > 'z' ? 'z' : (c < 'a' ? 'a' : c);

        last_spectrum += (char)c;
    }

#ifdef DEBUG
    cerr << "spectrum [" << last_spectrum << "] "
        << endl;
    cerr << "power rating = " << spectrum_power(last_spectrum) << endl;
#endif

    if (have_spectrums > 100)
        immsdb.set_spectrum(last_spectrum);
}
