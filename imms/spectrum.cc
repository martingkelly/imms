#include <math.h>
#include <assert.h>

#include <iostream>
#include <utility>
#include <fstream>

#include "spectrum.h"

using std::endl;
using std::cerr;
using std::pair;

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

static inline int offset2bpm(int offset)
{
    return ROUND(60 * SAMPLES/ (float)(MINBEATLENGTH + offset));
}

static inline bool roughly_double(int a, int b)
{
    return abs(a - 2 * b) < 6;
}

int BeatKeeper::peak_finder_helper(vector<int> &peaks, int min,
        int max, float cutoff)
{
    int count = 0;
    for (int i = min; i < max; ++i)
    {
        int index = 0;
        float local_max = 0;
        while (i < max && (beats[i] > cutoff
                    || (i + 1 < max && beats[i + 1] > cutoff)))
        {
            if (beats[i] > local_max)
            {
                index = i;
                local_max = beats[i];
            }
            ++i;
        }

        if (local_max)
        {
            ++count;
            cerr << " >> found peak @ " << offset2bpm(index) << endl;
            peaks.push_back(offset2bpm(index));
        }
    }
    return count;
}

int BeatKeeper::getBPM()
{
    cerr << samples << " samples in " << last - first << " seconds: "
        << ROUND(samples / (float)(last - first)) << " samples/sec" << endl; 

    // smooth ends so we don't confuse the peak finder
    int i = 0, val = 0;
    while (i < BEATSSIZE && beats[i] >= beats[i + 1]) ++i;
    val = i;
    while (i--) beats[i] = beats[val];

    i = BEATSSIZE - 1;
    while (i && beats[i] >= beats[i - 1]) --i;
    val = i - 1;
    while (++i < BEATSSIZE) beats[i] = beats[val];

    std::ofstream bstats("/tmp/beats", std::ios::trunc);

    for (int i = 0; i < BEATSSIZE; ++i)
        bstats << offset2bpm(i) << " " << ROUND(beats[i]) << endl;

    float max = 0, min = INT_MAX;
    for (int i = 0; i < BEATSSIZE; ++i)
    {
        if (beats[i] > max)
            max = beats[i];
        if (beats[i] < min)
            min = beats[i];
    }

    // look at the top 20%
    float cutoff = min + (max - min) * 0.80;

    int totalpeaks = 0;
    vector<int> slowpeaks;
    vector<int> fastpeaks;
    // offset = 38 --> 93 bpm
    totalpeaks += peak_finder_helper(slowpeaks, 38, BEATSSIZE, cutoff);
    totalpeaks += peak_finder_helper(fastpeaks, 0, 38, cutoff);

    reset();

    if (!totalpeaks)
        return -1;

    if (totalpeaks == 1)
        return fastpeaks.empty() ? slowpeaks.front() : fastpeaks.front();

    if (!fastpeaks.empty())
        for (vector<int>::iterator i = slowpeaks.begin();
                i != slowpeaks.end(); ++i)
            if (roughly_double(fastpeaks.front(), *i))
                return fastpeaks.front();

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
    { 11000, 5000, 4200, 3500, 2600, 2100, 1650, 1200, 1000, 780, 550,
        400, 290, 200, 110, 40 };

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
