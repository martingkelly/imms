#ifndef __SPECTRUM_H
#define __SPECTRUM_H

#include <stdint.h>
#include <time.h>

#include "immsconf.h"
#include "immsbase.h"

static const int short_spectrum_size = 16;
static const int long_spectrum_size = 256;

class BeatKeeper
{
public:
    BeatKeeper();
    void integrate_bass(int bass);
    int get_BPM(time_t seconds);
private:
    bool above_average;
    int max_bass, average_bass, beats;
};

class SpectrumAnalyzer : virtual protected ImmsBase
{
public:
    SpectrumAnalyzer();
    void start_spectrum_analysis();
    void stop_spectrum_analysis();
    void integrate_spectrum(uint16_t _long_spectrum[long_spectrum_size]);
protected:
    void finalize_spectrum();
    void reset();

    int bpm;

private:
    int have_spectrums;
    double long_spectrum[long_spectrum_size];
    time_t spectrum_start_time;
    string last_spectrum;

    BeatKeeper bpm0, bpm1;
};

#endif
