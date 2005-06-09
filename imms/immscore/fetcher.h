#ifndef __FETCHER_H
#define __FETCHER_H

#include "immsconf.h"
#include "immsdb.h"
#include "song.h"

class InfoFetcher : virtual protected ImmsDb
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

        Rating rating;
        int position, effective_rating;
        int relation, bpmrating, specrating;
        time_t last_played;
        bool identified;
    };

    virtual bool fetch_song_info(SongData &data);
    virtual bool parse_song_info(const SongData &data, StringPair &info);

    bool identify_playlist_item(int pos);

    int next_sid, pl_length;
};

#endif
