#ifndef __SPECTRUM_H
#define __SPECTRUM_H

#include <stdint.h>

#include <string>

#include "immsconf.h"
#include "immsbase.h"

using std::string;

#define MINBPM 45
#define MAXBPM 225
#define SECOND 75
#define MINBEATLENGTH (SECOND*60/MAXBPM)
#define MAXBEATLENGTH (SECOND*60/MINBPM)
#define SHORTSPECTRUM 16
#define LONGSPECTRUM 256

int spectrum_power(const string &spectstr);

#include <iostream>

using std::cerr;
using std::endl;

class BeatKeeper
{
public:
    BeatKeeper()
    {
        reset();
    }
    void reset()
    {
        memset(data, 0, sizeof(data));
        memset(beats, 0, sizeof(beats));
        current_position = current_window = data;
        last_window = &data[MAXBEATLENGTH];
    }
    void getBPM()
    {
        float max = 0, sum = 0;
        int max_index = 0;
        for (int i = 0; i < MAXBEATLENGTH - MINBEATLENGTH; ++i)
        {
            sum += beats[i];
            cerr << 60 * SECOND / (MINBEATLENGTH + i)
                << " " << ROUND(beats[i]) << endl;
            if (beats[i] < max)
                continue;

            max = beats[i];
            max_index = i;
        }

        cerr << "maximum " << max << "@"
            << MINBEATLENGTH + max_index << endl;
        cerr << "average " << sum /
                (MAXBEATLENGTH - MINBEATLENGTH - 1) << endl;
        cerr << " bpm is " << 60 * SECOND / (MINBEATLENGTH + max_index)
            << endl;
        reset();
    }
    void integrate_beat(float power)
    { 
        *current_position++ = power;
        if (current_position - current_window == MAXBEATLENGTH)
            process_window();
    }
    inline float wrap_pointer(int offset)
    {
        if (offset >= MAXBEATLENGTH)
            return *(current_window + offset - MAXBEATLENGTH);
        return *(last_window + offset);
    }
    void process_window()
    {
        // update beat values
        for (int i = 0; i < MAXBEATLENGTH; ++i)
        {
            for (int offset = MINBEATLENGTH; offset < MAXBEATLENGTH; ++offset)
            {
                beats[offset - MINBEATLENGTH] += 
                    last_window[i] * wrap_pointer(i + offset);
            }
        }

        // swap the windows
        float *tmp = current_window;
        current_window = current_position = last_window;
        last_window = tmp;
    }

protected:
    float *last_window, *current_window, *current_position;
    float data[2*MAXBEATLENGTH];
    float beats[MAXBEATLENGTH-MINBEATLENGTH];
};

class SpectrumAnalyzer : virtual protected ImmsBase
{
public:
    SpectrumAnalyzer();
    void integrate_spectrum(uint16_t long_spectrum[LONGSPECTRUM]);
    void finalize();

protected:
    float evaluate_transition(const string &from, const string &to);
    string get_last_spectrum() { return last_spectrum; }
    void reset();

private:
    BeatKeeper bpm;
    int have_spectrums;
    double spectrum[SHORTSPECTRUM];
    string last_spectrum;
};

#endif
