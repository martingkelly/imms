#include <math.h>
#include <assert.h>

#include <iostream>
#include <utility>
#include <fstream>
#include <map>

#include <utils.h>

#include "spectrum.h"

using std::endl;
using std::cerr;
using std::pair;
using std::map;

inline int freq2bark(int f) { return (int)(26.81 / (1 + (1960.0 / f)) - 0.53); }
inline int bark2freq(int b) { return (int)(1960 / (26.81 / (b + 0.53) - 1)); }

inline int indx2freq(int i) { return i * SAMPLERATE / WINDOWSIZE; }
inline int freq2indx(int i) { return (int)(i * WINDOWSIZE / SAMPLERATE) + 1; }

float scales[] = { 0.6619, 1.384, 1.918, 1.493, 3.261, 2.417, 2.756,
    2.45, 2.778, 2.915, 2.476, 2.497, 2.052, 2.213, 1.885, 1.859, 1.931,
    1.65, 1.525, 1.506, 1.302, 2.376, 23.1 };

float bias[] = { 164.3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0 };

string encode_spectrum(float spectrum[BARKSIZE])
{
    string spec;
    for (int i = 0; i < BARKSIZE; ++i)
    {
        char c = 'a' + ROUND((spectrum[i] - bias[i]) * scales[i]);
        c = std::min('z', std::max('a', c));

        spec += c;
    }
    return spec;
}

void freq2bark(float freqs[NFREQS], float bark[BARKSIZE])
{
    for (int i = 0; i < NFREQS; )
    {
        int b = freq2bark(indx2freq(i));
        int last = std::min(freq2indx(bark2freq(b + 1)), NFREQS);

        bark[b] = 0;
        for (int j = i; j < last; ++j)
            bark[b] += freqs[j];
        //bark[b] /= (float)(last - i);

        i = last;
    }
}

void BeatKeeper::reset()
{
    samples = 0;
    memset(data, 0, sizeof(data));
    memset(beats, 0, sizeof(beats));
    current_position = current_window = data;
    last_window = &data[MAXBEATLENGTH];
}

const BeatKeeper &BeatKeeper::operator +=(const BeatKeeper &other)
{
    float my_max = 0, my_min = INT_MAX;
    float other_max = 0, other_min = INT_MAX;

    for (int i = 0; i < BEATSSIZE; ++i)
    {
        if (beats[i] > my_max)
            my_max = beats[i];
        if (beats[i] < my_min)
            my_min = beats[i];

        if (other.beats[i] > other_max)
            other_max = other.beats[i];
        if (other.beats[i] < other_min)
            other_min = other.beats[i];
    }

    float my_range = my_max - my_min == 0 ? 1 : my_max - my_min;
    float other_range = other_max - other_min == 0 ? 1 : other_max - other_min;

    for (int i = 0; i < BEATSSIZE; ++i)
        beats[i] =
            100 * (beats[i] - my_min) / my_range +
            100 * (other.beats[i] - other_min) / other_range;

    return *this;
}

static inline int offset2bpm(int offset)
{
    return ROUND(60 * WINDOWSPSEC / (float)(MINBEATLENGTH + offset));
}

static inline int bpm2offset(int bpm)
{
    return ROUND(60 * WINDOWSPSEC / (float)bpm  - MINBEATLENGTH);
}

static inline bool roughly_double(int a, int b)
{
    return abs(a - 2 * b) <= 6;
}

float BeatKeeper::check_peak(int index)
{
    float max_pos_dist = 0, max_neg_dist = 0;
    int pos_had_ups = 0, neg_had_ups = 0;
    for (int i = 2; index + i < BEATSSIZE && i < 8; ++i)
    {
        if (pos_had_ups < 2 && index + i < BEATSSIZE 
                && beats[index] - beats[index + i] > max_pos_dist)
            max_pos_dist = beats[index] - beats[index + i];
        if (neg_had_ups < 2 && index - i > -1
                && beats[index] - beats[index - i] > max_neg_dist)
            max_neg_dist = beats[index] - beats[index - i];

        pos_had_ups += beats[index + i] > beats[index + i - 1];
        neg_had_ups += beats[index - i] > beats[index - i + 1];
    }

    return max_pos_dist > max_neg_dist ? max_neg_dist : max_pos_dist;
}

int BeatKeeper::peak_finder_helper(vector<int> &peaks, int min,
        int max, float cutoff, float range)
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

        if (local_max && check_peak(index) > range * 0.2)
        {
            ++count;
#ifdef DEBUG
            cerr << " >> found peak @ " << offset2bpm(index) << endl;
#endif
            peaks.push_back(offset2bpm(index));
        }
    }
    return count;
}

int BeatKeeper::maybe_double(int bpm, float min, float range)
{
    vector<int> dpeaks;
    int i = bpm2offset(bpm * 2);
    int found = peak_finder_helper(dpeaks, i - 5, i + 5,
            min + range / 2, range * 0.35);

    if (found == 1)
        return dpeaks.front();
    return bpm;
}

void BeatKeeper::dump(const string &filename)
{
    std::ofstream bstats(filename.c_str(), std::ios::trunc);

    for (int i = 0; i < BEATSSIZE; ++i)
        bstats << offset2bpm(i) << " " << ROUND(beats[i]) << endl;

    bstats.close();
}

string BeatKeeper::get_bpm_graph()
{
    bool empty = true;
    string graph;
    for (int i = 3; i < BEATSSIZE; i += 4)
    {
        int c = 'a' + ROUND((beats[i] + beats[i-1]
                    + beats[i-2] + beats[i-3]) / 40);
        if (c != 'a')
            empty = false;
        graph += (char)(c > 'z' ? 'z' : c);
    }
    return empty ? "" : graph;
}

int BeatKeeper::guess_actual_bpm()
{
#if 0
    cerr << samples << " samples in " << last - first << " seconds: "
        << ROUND(samples / (float)(last - first)) << " samples/sec" << endl;
#endif

    float max = 0, min = INT_MAX;
    for (int i = 0; i < BEATSSIZE; ++i)
    {
        if (beats[i] > max)
            max = beats[i];
        if (beats[i] < min)
            min = beats[i];
    }

    // look at the top 20%
    float range = max - min;
    float cutoff = min + range * 0.80;

    int totalpeaks = 0;
    vector<int> slowpeaks;
    vector<int> fastpeaks;
    // offset = 38 --> 93 bpm
    totalpeaks += peak_finder_helper(slowpeaks, 38, BEATSSIZE, cutoff, range);
    totalpeaks += peak_finder_helper(fastpeaks, 0, 38, cutoff, range);

    reset();

    if (!totalpeaks)
        return 0;

    if (totalpeaks == 1)
    {
        if (!fastpeaks.empty())
            return fastpeaks.front();

        int slow = slowpeaks.front();
        return maybe_double(slow, min, range);
    }

    int peak = 0, count = 0;
    // see if only one of the slow peaks doubles well
    if (fastpeaks.empty())
    {
        for (vector<int>::iterator i = slowpeaks.begin();
                i != slowpeaks.end(); ++i)
        {
            int dbl = maybe_double(*i, min, range);
            if (*i != dbl)
            {
                peak = dbl;
                ++count;
            }
        }
        if (count == 1)
            return peak;
    }
    // look for peaks that are at double the bpm from each other
    else
    {
        for (vector<int>::iterator i = fastpeaks.begin();
                i != fastpeaks.end(); ++i)
            for (vector<int>::iterator j = slowpeaks.begin();
                    j != slowpeaks.end(); ++j)
                if (roughly_double(*i, *j))
                {
                    peak = *i;
                    ++count;
                }
        if (count == 1)
            return peak;
    }

    // see if one of the peaks is more pronounced than others
    map<float, int> powers;
    for (vector<int>::iterator i = slowpeaks.begin();
            i != slowpeaks.end(); ++i)
        powers[check_peak(bpm2offset(*i))] = *i;
    for (vector<int>::iterator i = fastpeaks.begin();
            i != fastpeaks.end(); ++i)
        powers[check_peak(bpm2offset(*i))] = *i;

    float first = powers.begin()->first;
    if (first / 2 > (++powers.begin())->first)
        return powers.begin()->second;

    // see if there is only one peak in fasts below ~145
    if (fastpeaks.size() == 1 || (fastpeaks.size() > 1
                && fastpeaks[0] <= 145 && fastpeaks[1] > 145))
        return fastpeaks.front();

    return 0;
}

void BeatKeeper::integrate_beat(float power)
{
    *current_position++ = power;
    if (current_position - current_window == MAXBEATLENGTH)
        process_window();
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

SpectrumAnalyzer::SpectrumAnalyzer()
{
    last_bpm = last_spectrum = "";
    reset();
}

void SpectrumAnalyzer::reset()
{
    have_spectrums = 0;
    memset(spectrum, 0, sizeof(spectrum));

    bpm_low.reset();
    bpm_hi.reset();
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

float rms_string_distance(const string &s1, const string &s2)
{
    if (s1 == "" || s2 == "")
        return 0;

    unsigned len = s1.length();
    assert(len == s2.length());
    float distance = 0;

    for (int i = 0; i < SHORTSPECTRUM; ++i)
        distance += pow(s1[i] - s2[i], 2);

    return sqrt(distance / len);
}

float SpectrumAnalyzer::color_transition(const string &from,
        const string &to)
{
    float rms = rms_string_distance(from, to);
    if (rms)
        cerr << "spectrum distance [" << from << "]  [" << to << "] = "
            << rms << endl;
    return 0;
}

float SpectrumAnalyzer::bpm_transition(const string &from,
        const string &to)
{
    float rms = rms_string_distance(from, to);
    if (rms)
        cerr << "bpm distance [" << from << "]  [" << to << "] = "
            << rms << endl;
    return 0;
}

void SpectrumAnalyzer::integrate_spectrum(float long_spectrum[LONGSPECTRUM])
{
    float bark[BARKSIZE];
    freq2bark(long_spectrum, bark);

    float power = 0;
    for (int i = 0; i < 3; ++i)
        power += bark[i];

    bpm_low.integrate_beat(power / 70);

    power = 0;
    for (int i = BARKSIZE; i > BARKSIZE - 3; --i)
        power += bark[i];

    bpm_hi.integrate_beat(power * 10);

    for (int i = 0; i < BARKSIZE; ++i)
        spectrum[i] = (spectrum[i] * have_spectrums + bark[i])
            / (float)(have_spectrums + 1);
    ++have_spectrums;
}

void SpectrumAnalyzer::finalize()
{
    BeatKeeper bpm_com;

    bpm_com += bpm_low; 
    bpm_com += bpm_hi; 

    bpm_low.dump("/tmp/beats-low");
    bpm_hi.dump("/tmp/beats-high");
    bpm_com.dump("/tmp/beats-com");

    last_bpm = bpm_com.get_bpm_graph();
    int bpmval = bpm_com.guess_actual_bpm();
    cerr << "BPM [com] = " << bpmval << endl;

    if (!have_spectrums)
        return;

    last_spectrum = encode_spectrum(spectrum);

#if 0
    std::ofstream bstats("/tmp/spectrums", std::ios::app);
    for (int i = 0; i < BARKSIZE; ++i)
        bstats << spectrum[i] << " ";
    bstats << endl;
    bstats.close();
#endif

#ifdef DEBUG
    cerr << "spectrum  [" << last_spectrum << "] " << endl;
    cerr << "bpm graph [" << last_bpm << "] " << endl;
#endif
}
