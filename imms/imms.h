#ifndef __IMMS_H
#define __IMMS_H

#include <string>
#include <fstream>
#include <list>

#include "picker.h"
#include "xidle.h"
#include "server.h"

// IMMS, UMMS, we all MMS for XMMS?

class Imms : public SongPicker,
             protected XIdle,
             protected ImmsServer
{
public:
    Imms();
    ~Imms();

    // Important inherited public methods
    //  SongPicker:
    //      int select_next()
    //      bool add_candidate(int, bool)

    void start_song(int position, const std::string &path);
    void end_song(bool at_the_end, bool jumped, bool bad);

    // get the last song played
    int  get_previous();

    virtual void playlist_changed();

    // process internal events - call this periodically
    void do_events();
    void do_idle_events();

    // configure imms
    void setup(bool use_xidle);

protected:
    struct LastInfo {
        time_t set_on;
        int sid;
    };

    void reset_selection();

    // Helper functions
    bool fetch_song_info(SongData &data);
    void print_song_info();
    void set_lastinfo(LastInfo &last);
    void evaluate_transition(SongData &data, LastInfo &last, float weight);

    // State variables
    bool last_skipped, last_jumped;
    int local_max;

    std::list<int> history;
    std::ofstream fout;

    LastInfo handpicked, last;
};

#endif
