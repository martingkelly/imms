#ifndef __SPECTRUM_H
#define __SPECTRUM_H

#include <stdint.h>

#include <string>

#include "immsconf.h"
#include "immsbase.h"

using std::string;

static const int short_spectrum_size = 16;
static const int long_spectrum_size = 256;

class SpectrumAnalyzer : virtual protected ImmsBase
{
public:
    SpectrumAnalyzer();
    void integrate_spectrum(uint16_t _long_spectrum[long_spectrum_size]);
    void finalize();

protected:
    float evaluate_transition(const string &from, const string &to);
    string get_last_spectrum() { return last_spectrum; }
    void reset();

private:
    int have_spectrums;
    double long_spectrum[long_spectrum_size];
    string last_spectrum;
};

#endif
