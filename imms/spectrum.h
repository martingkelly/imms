#ifndef __SPECTRUM_H
#define __SPECTRUM_H

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include <string>
#include <vector>

#include "immsconf.h"
#include "immsbase.h"

using std::string;
using std::vector;

#define MINBPM          45
#define MAXBPM          225
#define SAMPLES         100
#define MINBEATLENGTH   (SAMPLES*60/MAXBPM)
#define MAXBEATLENGTH   (SAMPLES*60/MINBPM)
#define BEATSSIZE       (MAXBEATLENGTH-MINBEATLENGTH)
#define SHORTSPECTRUM   16
#define LONGSPECTRUM    256
#define MICRO           1000000

class BeatKeeper
{
public:
    BeatKeeper() { reset(); }
    void reset();
    int getBPM();
    void integrate_beat(float power);

protected:
    void process_window();
    int peak_finder_helper(vector<int> &peaks,
            int min, int max, float cutoff);

    struct timeval prev;
    long unsigned int samples;
    time_t first, last;
    float average_with, *last_window, *current_window, *current_position;
    float data[2*MAXBEATLENGTH];
    float beats[BEATSSIZE];
};

class SpectrumAnalyzer : virtual protected ImmsBase
{
public:
    SpectrumAnalyzer();
    void integrate_spectrum(uint16_t long_spectrum[LONGSPECTRUM]);
    void finalize();

protected:
    float evaluate_transition(const string &from, const string &to);
    const string& get_last_spectrum() { return last_spectrum; }
    void reset();

private:
    BeatKeeper bpm;
    int have_spectrums;
    double spectrum[SHORTSPECTRUM];
    string last_spectrum;
};

#endif
