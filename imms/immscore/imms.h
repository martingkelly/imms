#ifndef __IMMS_H
#define __IMMS_H

#include <string>
#include <fstream>
#include <memory>

#include "picker.h"
#include "xidle.h"
#include "serverstub.h"

// IMMS, UMMS, we all MMS for XMMS?

class Imms : public SongPicker,
             protected XIdle
{
public:
    Imms(IMMSServer *server);
    ~Imms();

    // Important inherited public methods
    //  SongPicker:
    //      int select_next()
    //      bool add_candidate(int, bool)

    void start_song(int position, std::string path);
    void end_song(bool at_the_end, bool jumped, bool bad);

    virtual void request_playlist_item(int index);
    void playlist_changed(int length);
    virtual void playlist_ready();

    // process internal events - call this periodically
    void do_events();

    // configure imms
    void setup(bool use_xidle);

    friend class ImmsProcessor;

protected:
    struct LastInfo {
        LastInfo() : sid(-1) {}
        time_t set_on;
        int sid;
        StringPair acoustic;
    };

    void reset_selection();
    void get_metacandidates();

    // Helper functions
    bool fetch_song_info(SongData &data);
    void print_song_info();
    void set_lastinfo(LastInfo &last);
    void evaluate_transition(SongData &data, LastInfo &last, float weight);

    // State variables
    bool last_skipped, last_jumped;
    int local_max;

    std::ofstream fout;

    LastInfo handpicked, last;
    IMMSServer *server;
};

#endif
