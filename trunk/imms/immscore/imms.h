#ifndef __IMMS_H
#define __IMMS_H

#include <string>
#include <fstream>
#include <memory>

#include "picker.h"
#include "xidle.h"
#include "serverstub.h"
#include <analyzer/mfcckeeper.h>
#include <analyzer/beatkeeper.h>
#include <model/model.h>

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

    virtual void playlist_ready();

    void playlist_changed(int length);

    // process internal events - call this periodically
    void do_events();

    // configure imms
    void setup(bool use_xidle);

    void sync(bool incharge);

    friend class ImmsProcessor;

protected:
    struct LastInfo {
        LastInfo() : sid(-1), avalid(false) {}
        time_t set_on;
        int uid, sid;
        bool avalid;
        MixtureModel mm;
        float beats[BEATSSIZE];
    };

    virtual void playlist_updated() { server->playlist_updated(); }

    // Implementations for SongPicker
    virtual void request_playlist_item(int index);
    virtual void get_metacandidates(int size);
    virtual void reset_selection();

    // Helper functions
    bool fetch_song_info(SongData &data);
    void print_song_info();
    void set_lastinfo(LastInfo &last);
    void evaluate_transition(SongData &data, LastInfo &last, float weight);

    // State variables
    bool last_skipped, last_jumped;
    int local_max;

    std::ofstream fout;

    SVMSimilarityModel model;
    LastInfo handpicked, last;
    IMMSServer *server;
};

#endif
