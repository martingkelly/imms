#ifndef __IMMS_H
#define __IMMS_H

#include <string>
#include <fstream>
#include <memory>
#include <list>

#include "immsdb.h"
#include "songinfo.h"
#include "xidle.h"

using std::string;
using std::ofstream;
using std::auto_ptr;
using std::list;

#define ROUND(x) (int)((x) + 0.5)

int imms_random(int max);

class SongData
{
public:
    SongData(int _position = -1, const string &_path = "");
    bool operator ==(const SongData &other) const
        { return position == other.position; }

    IntPair id;
    int position, rating, relation, composite_rating; 
    bool identified, unrated;
    time_t last_played;
    string path;
};

typedef list<SongData> Candidates;

// IMMS, UMMS, we all MMS for XMMS?

class Imms
{
public:
    Imms();

    // Public interface
    int  select_next();
    bool add_candidate(int playlist_num, string path, bool urgent = false);

    void start_song(const string &path);
    void end_song(bool at_the_end, bool jumped, bool bad);

    void playlist_changed(int _playlist_size);
    void pump();

    void setup(const char* _email, bool _use_xidle);

protected:
    // Helper functions
    bool fetch_song_info(SongData &data);
    bool parse_song_info(const string &path);
    void print_song_info();

    // State variables
    string email;
    bool last_skipped, last_jumped, use_xidle, winner_valid;
    int have_candidates, attempts;
    int last_handpicked, playlist_size, current_rating;

    // Helper objects
    auto_ptr<ImmsDb> immsdb;
    ofstream fout;
    SongInfo songinfo;
    SongData winner;
    Candidates candidates;
    XIdle xidle;

    // Song info cache
    string artist, title;
};


#endif
