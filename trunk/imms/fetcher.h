#ifndef __FETCHER_H
#define __FETCHER_H

#include "immsconf.h"
#include "immsbase.h"
#include "songinfo.h"

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

class InfoFetcher : virtual protected ImmsBase, private SongInfo
{
protected:
    virtual bool fetch_song_info(SongData &data);
    virtual bool parse_song_info(const string &path, string &title);
    string email;
};

#endif
