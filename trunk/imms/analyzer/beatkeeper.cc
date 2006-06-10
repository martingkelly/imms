#include <string.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>

#include "beatkeeper.h"
#include "immsutil.h"

using std::string;
using std::endl;
using std::vector;
using std::map;

using std::cerr;

bool BeatKeeper::extract_features(float *beats, vector<float> &features)
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
   
    if (max == 0 || max == min)
        return false;

    features.push_back(max);                                // max
    features.push_back(min / max);                          // relative min
    features.push_back(sum / ((max - min) * BEATSSIZE));    // relative area

    map<int, int> allpeaks;

    float cutoff = max - (max - min) * 0.2;
    float localmax = 0; 
    int maxindex = 0, width = 0;
    for (int i = BEATSSIZE - 1; i >= 0; --i)
    {
        if (beats[i] < cutoff)
        {
            if (!width)
                continue;

            int realwidth = OFFSET2BPM(i - width) - OFFSET2BPM(i);
            allpeaks[realwidth] = OFFSET2BPM(maxindex);
            localmax = 0;
            width = 0;
            continue;
        }

        if (beats[i] > localmax)
        {
            localmax = beats[i];
            maxindex = i;
        }
        ++width;
    }

    map<int, int> peaks;
    map<int, int>::reverse_iterator i = allpeaks.rbegin();
    for (int j = 0; j < 3; ++j, ++i)
    {
        if (i == allpeaks.rend())
            break;
        peaks[i->first] = i->second;
    }

    int first_peak = 0; 
    for (map<int, int>::iterator i = peaks.begin(); i != peaks.end(); ++i)
    {
        if (!first_peak)
            features.push_back(first_peak = i->second);
        else
            features.push_back(i->second / (float)first_peak);
        features.push_back(beats[i->second] / max);
        features.push_back(i->first);
    }

    for (int i = peaks.size(); i < 3; ++i)
    {
        features.push_back(0);
        features.push_back(0);
        features.push_back(0);
    }

#if defined(DEBUG) && 0
    cerr << "Kept peaks" << endl;
    for (map<int, int>::iterator i = peaks.begin(); i != peaks.end(); ++i)
        cerr << " -> @ " << i->second << " = " << i->first << endl;
#endif

    return true;
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
        bstats << OFFSET2BPM(i) << " " << ROUND(beats[i]) << endl;

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

float *BeatManager::get_result()
{
    return lofreq.beats;
}
