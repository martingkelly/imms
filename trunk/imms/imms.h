#ifndef __IMMS_H
#define __IMMS_H

#include <string>
#include <fstream>

#include "picker.h"
#include "plugin.h"
#include "xidle.h"

using std::string;
using std::ofstream;

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

// IMMS, UMMS, we all MMS for XMMS?

class Imms : public SongPicker
{
public:
    Imms();

    void start_song(const string &path);
    void end_song(bool at_the_end, bool jumped, bool bad);

    void start_spectrum_analysis();
    void stop_spectrum_analysis();
    void integrate_spectrum(uint16_t _long_spectrum[long_spectrum_size]);

    void playlist_changed(int _playlist_size);
    void pump();

    void setup(const char* _email, bool _use_xidle);

protected:
    // Helper functions
    bool fetch_song_info(SongData &data);
    void print_song_info();
    void finalize_spectrum();

    // State variables
    bool last_skipped, last_jumped, use_xidle;
    int local_max, have_spectrums, last_handpicked, playlist_size;
    double long_spectrum[long_spectrum_size];
    time_t spectrum_start_time;
    string last_spectrum;

    int bpm;

    // Helper objects
    ofstream fout;
    BeatKeeper bpm0, bpm1;
    XIdle xidle;
};

#endif
