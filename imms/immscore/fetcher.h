#ifndef __FETCHER_H
#define __FETCHER_H

#include "immsconf.h"
#include "immsdb.h"
#include "songinfo.h"
#include "song.h"

#define     NEW_PLAYS               2
#define     NEW_BONUS               20

#define     TREND_FACTOR            3.0
#define     MAX_TREND               20

class InfoFetcher : virtual protected ImmsDb, private SongInfo
{
public:
    InfoFetcher() : next_sid(-1) {}
protected:
    class SongData : public Song
    {
    public:
        SongData(int _position, const string &_path);
        bool operator ==(const SongData &other) const
            { return position == other.position; }

        void reset();
        bool get_song_from_playlist();

        int position, rating, composite_rating;
        int relation, bpmrating, specrating, newness;
        time_t last_played;
        bool identified;
        float trend_scale;
    };

    virtual bool fetch_song_info(SongData &data);
    virtual bool parse_song_info(const string &path, StringPair &info);

    bool identify_playlist_item(int pos);

    int next_sid, pl_length;
};

#endif
