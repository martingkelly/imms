#ifndef __SPECTRUM_H
#define __SPECTRUM_H

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include <string>
#include <vector>

#include <immsconf.h>
#include <song.h>

using std::string;
using std::vector;

#define MINBPM          50
#define MAXBPM          250

#define WINDOWSIZE      512
#define OVERLAP         256
#define SAMPLERATE      22050
#define NUMMEL          40
#define NUMCEPSTR       10
#define NUMGAUSS        3

#define WINPERSEC       (SAMPLERATE / (WINDOWSIZE - OVERLAP)) 
#define BEATSSIZE       (MAXBEATLENGTH-MINBEATLENGTH)
#define READSIZE        (WINDOWSIZE - OVERLAP)
#define NUMFREQS        (WINDOWSIZE / 2 + 1)
#define MAXFREQ         (SAMPLERATE / 2)
#define FREQDELTA       ROUND(MAXFREQ / (float)NUMFREQS)
#define MINFREQ         FREQDELTA
#define BARKSIZE        (int)(26.81/(1+(2*1960.0/SAMPLERATE)) + 0.47)
#define SHORTSPECTRUM   BARKSIZE
#define LONGSPECTRUM    NUMFREQS

#define MINBEATLENGTH   (WINPERSEC*60/MAXBPM)
#define MAXBEATLENGTH   (WINPERSEC*60/MINBPM)

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
    SpectrumAnalyzer(const string &path);
    ~SpectrumAnalyzer() { finalize(); }
    void integrate_spectrum(const vector<double> &long_spectrum);
    void finalize();

    bool is_known();

protected:
    void reset();

private:
    Song song;
    BeatKeeper bpm_low, bpm_hi;
    int have_spectrums;
    float spectrum[SHORTSPECTRUM];
};

#endif
