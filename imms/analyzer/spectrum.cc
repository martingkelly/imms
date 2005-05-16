#include <math.h>
#include <assert.h>

#include <iostream>
#include <utility>
#include <fstream>
#include <map>

#include <immsutil.h>
#include <sqlite++.h>

#include "spectrum.h"

using std::endl;
using std::cerr;
using std::pair;
using std::map;

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
    return ROUND(60 * WINPERSEC / (float)(MINBEATLENGTH + offset));
}

static inline int bpm2offset(int bpm)
{
    return ROUND(60 * WINPERSEC / (float)bpm  - MINBEATLENGTH);
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
    for (int i = 3; i < BEATSSIZE; i += 3)
    {
        int c = 'a' + ROUND((beats[i] + beats[i-1] + beats[i-2]) / 30);
        if (c != 'a')
            empty = false;
        graph += std::min((char)c, 'z');
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

SpectrumAnalyzer::SpectrumAnalyzer(const string &path) : song(path)
{
    if (!song.isok())
        throw string("identify error!");
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

void SpectrumAnalyzer::integrate_spectrum(const vector<double> &long_spectrum)
{
    float bark[BARKSIZE];
    //freq2bark(long_spectrum, bark);

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

    string bpmgraph = bpm_com.get_bpm_graph();

#ifdef DEBUG
    bpm_low.dump("/tmp/beats-low");
    bpm_hi.dump("/tmp/beats-high");
    bpm_com.dump("/tmp/beats-com");
#endif

    if (!have_spectrums)
        return;

    //song.set_acoustic(encode_spectrum(spectrum), bpmgraph);
}

bool SpectrumAnalyzer::is_known()
{
    try {
        StringPair p = song.get_acoustic();
        return p.first != "" && p.second != "";
    }
    WARNIFFAILED();

    return false;
}
