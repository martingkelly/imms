#ifndef __SPECTRUM_H
#define __SPECTRUM_H

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include <string>
#include <vector>

#include "immsconf.h"

using std::string;
using std::vector;

#define MINBPM          50
#define MAXBPM          250

#define WINDOWSIZE      512
#define OVERLAP         256
#define SAMPLERATE      22050

#define WINDOWSPSEC     (SAMPLERATE/(WINDOWSIZE-OVERLAP)) 
#define MINBEATLENGTH   (WINDOWSPSEC*60/MAXBPM)
#define MAXBEATLENGTH   (WINDOWSPSEC*60/MINBPM)
#define BEATSSIZE       (MAXBEATLENGTH-MINBEATLENGTH)
#define READSIZE        (WINDOWSIZE - OVERLAP)
#define NFREQS          (WINDOWSIZE / 2 + 1)
#define BARKSIZE        (int)(26.81/(1+(2*1960.0/SAMPLERATE)) + 0.47)
#define SHORTSPECTRUM   BARKSIZE
#define LONGSPECTRUM    NFREQS

float rms_string_distance(const string &s1, const string &s2);

class BeatKeeper
{
public:
    BeatKeeper() { reset(); }
    void reset();
    void dump(const string &filename);
    string get_bpm_graph();
    int guess_actual_bpm();
    void integrate_beat(float power);
    const BeatKeeper &operator +=(const BeatKeeper &other);

protected:
    void process_window();
    float check_peak(int i);
    int maybe_double(int bpm, float min, float range);
    int peak_finder_helper(vector<int> &peaks, int min, int max,
            float cutoff, float range);

    long unsigned int samples;
    float average_with, *last_window, *current_window, *current_position;
    float data[2*MAXBEATLENGTH];
    float beats[BEATSSIZE];
};

class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();
    void integrate_spectrum(float long_spectrum[LONGSPECTRUM]);
    void finalize();

protected:
    float color_transition(const string &from, const string &to);
    float bpm_transition(const string &from, const string &to);
    const string& get_last_spectrum() { return last_spectrum; }
    string get_last_bpm() { return last_bpm; }
    void reset();

private:
    BeatKeeper bpm_low, bpm_hi;
    int have_spectrums;
    float spectrum[SHORTSPECTRUM];

    string last_spectrum;
    string last_bpm;
};

#endif
