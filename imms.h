#ifndef __IMMS_H
#define __IMMS_H

#include <string>
#include <fstream>
#include <vector>

#include "picker.h"
#include "spectrum.h"
#include "xidle.h"

// IMMS, UMMS, we all MMS for XMMS?

class Imms : public SongPicker,
             public SpectrumAnalyzer,
             protected XIdle
{
public:
    Imms();

    // Important inherited public methods
    //  SongPicker:
    //      int select_next()
    //      bool add_candidate(int, bool)
    //  SpectrumAnalyzer:
    //      start_spectrum_analysis()
    //      stop_spectrum_analysis()
    //      integrate_spectrum(uint16_t[])

    void start_song(int position, const std::string &path);
    void end_song(bool at_the_end, bool jumped, bool bad);

    // get the last song played
    int  get_previous();

    void playlist_changed(int playlist_size);

    // process internal events - call this periodically
    void pump();

    // configure imms
    //  email is used for getting legacy ratings from id3 tags
    void setup(const char* _email, bool use_xidle, bool _use_autooff);

    bool is_enabled() { return state == ON; }

protected:
    // Helper functions
    int fetch_song_info(SongData &data);
    void print_song_info();

    // State variables
    bool last_skipped, last_jumped, use_autooff;
    int local_max;

    std::vector<int> history;
    std::ofstream fout;

    struct {
        std::string spectrum;
        int bpm, sid;
    } last_hp;
};

#endif
