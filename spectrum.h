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

#define MINBPM          50
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
    BeatKeeper(const string &_name) : name(_name) { reset(); }
    void reset();
    int getBPM();
    void integrate_beat(float power);
    const BeatKeeper &operator +=(const BeatKeeper &other);

protected:
    void process_window();
    float check_peak(int i);
    int maybe_double(int bpm, float min, float range);
    int peak_finder_helper(vector<int> &peaks, int min, int max,
            float cutoff, float range);

    string name;
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
    float color_transition(const string &from, const string &to);
    float bpm_transition(int from, int to);
    const string& get_last_spectrum() { return last_spectrum; }
    int get_last_bpm() { return last_bpm; }
    void reset();

private:
    BeatKeeper bpm_low, bpm_hi;
    int have_spectrums;
    double spectrum[SHORTSPECTRUM];
    string last_spectrum;
    int last_bpm;
};

#endif
