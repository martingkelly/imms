#include <string.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <utility>

#include "beatkeeper.h"
#include "immsutil.h"

using std::string;
using std::endl;
using std::vector;
using std::pair;

using std::cerr;

static inline int offset2bpm(int offset)
{
    return ROUND(60 * WINPERSEC / (float)(MINBEATLENGTH + offset));
}

void BeatKeeper::extract_features(float *beats, vector<float> &features)
{
    float sum = 0, min = 1e100, max = 0;
    for (int i = 0; i < BEATSSIZE; ++i)
    {
        if (beats[i] < min)
            min = beats[i];
        if (beats[i] > max)
            max = beats[i];
        sum += beats[i];
    }

    features.push_back(max);                                // max
    features.push_back(min / max);                          // relative min
    features.push_back(sum / ((max - min) * BEATSSIZE));    // relative area

    vector< pair<int, int> > peaks;

    float cutoff = max - (max - min) * 0.2;
    float localmax = 0; 
    int maxindex = 0, width = 0;
    for (int i = 0; i < BEATSSIZE; ++i)
    {
        if (beats[i] < cutoff)
        {
            if (!width)
                continue;

            int realwidth = offset2bpm(i) - offset2bpm(i - width);
            peaks.push_back(pair<int, int>(maxindex, realwidth));
            localmax = 0;
            width = 0;
        }

        if (beats[i] > localmax)
        {
            localmax = beats[i];
            maxindex = i;
        }
        ++width;
    }

    int first_peak = 0; 
    for (int i = 0; i < std::min((int)peaks.size(), 3); ++i)
    {
        if (!first_peak)
            features.push_back(first_peak = peaks[i].first);
        else
            features.push_back(peaks[i].first / (float)first_peak);
        features.push_back(beats[peaks[i].first] / max);
        features.push_back(peaks[i].second);
    }

    for (int i = peaks.size(); i < 3; ++i)
    {
        features.push_back(0);
        features.push_back(0);
        features.push_back(0);
    }

#ifdef DEBUG
    cerr << "Found peaks" << endl;
    for (unsigned i = 0; i < peaks.size(); ++i)
        cerr << " -> @ " << peaks[i].first << " = " << peaks[i].second << endl;
#endif
}

void BeatKeeper::reset()
{
    samples = 0;
    memset(data, 0, sizeof(data));
    memset(beats, 0, sizeof(beats));
    current_position = current_window = data;
    last_window = &data[MAXBEATLENGTH];
}

void BeatKeeper::dump(const string &filename)
{
    std::ofstream bstats(filename.c_str(), std::ios::trunc);

    for (int i = 0; i < BEATSSIZE; ++i)
        bstats << offset2bpm(i) << " " << ROUND(beats[i]) << endl;

    bstats.close();
}

void BeatKeeper::process(float power)
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

void BeatManager::process(const std::vector<double> &melfreqs)
{
    lofreq.process((melfreqs[0] + melfreqs[1]) / 1e11);
}

void BeatManager::finalize()
{
#ifdef DEBUG
    lofreq.dump("/tmp/lofreq");
#endif
}

void *BeatManager::get_result()
{
    return lofreq.beats;
}
