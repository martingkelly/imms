#ifndef __FETCHER_H
#define __FETCHER_H

#include "immsconf.h"
#include "immsdb.h"
#include "songinfo.h"
#include "song.h"

class InfoFetcher : virtual protected ImmsDb, private SongInfo
{
public:
    virtual void playlist_changed();

protected:
    class SongData : public Song
    {
    public:
        SongData(int _position = -1, const string &_path = "");
        bool operator ==(const SongData &other) const
            { return position == other.position; }

        int position, rating, relation;
        int composite_rating;
        bool identified, unrated;
        time_t last_played;
    };

    bool playlist_identify_item(int pos);
    virtual bool fetch_song_info(SongData &data);
    virtual bool parse_song_info(const string &path, string &title);

    int next_sid;
};

#endif
