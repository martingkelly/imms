#ifndef __IMMS_H
#define __IMMS_H

#include <string>
#include <fstream>

#include "picker.h"
#include "spectrum.h"
#include "plugin.h"
#include "xidle.h"

using std::string;
using std::ofstream;

// IMMS, UMMS, we all MMS for XMMS?

class Imms : public SongPicker, public SpectrumAnalyzer
{
public:
    Imms();

    void start_song(const string &path);
    void end_song(bool at_the_end, bool jumped, bool bad);

    void playlist_changed(int playlist_size);
    void pump();

    void setup(const char* _email, bool _use_xidle);

protected:
    // Helper functions
    bool fetch_song_info(SongData &data);
    void print_song_info();

    // State variables
    bool last_skipped, last_jumped, use_xidle;
    int local_max, last_handpicked;

    // Helper objects
    ofstream fout;
    XIdle xidle;
};

#endif
