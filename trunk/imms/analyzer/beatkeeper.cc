#include <string.h>
#include <fstream>
#include <iostream>

#include "beatkeeper.h"
#include "immsutil.h"

using std::string;
using std::endl;
using std::vector;

static inline int offset2bpm(int offset)
{
    return ROUND(60 * WINPERSEC / (float)(MINBEATLENGTH + offset));
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
    lofreq.dump("/tmp/lofreq");
}
