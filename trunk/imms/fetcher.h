#ifndef __FETCHER_H
#define __FETCHER_H

#include "immsconf.h"
#include "immsbase.h"
#include "songinfo.h"

class InfoFetcher : virtual protected ImmsBase, private SongInfo
{
protected:
    class SongData
    {
        public:
            SongData(int _position = -1, const string &_path = "");
            bool operator ==(const SongData &other) const
            { return position == other.position; }

            IntPair id;
            int position, rating, relation, bpm_value;
            int composite_rating, color_rating, bpm_rating;
            bool identified, unrated;
            time_t last_played;
            string path, spectrum;
    };

    virtual int fetch_song_info(SongData &data);
    virtual bool parse_song_info(const string &path, string &title);

    int next_sid;
    string email;
};

#endif
