/*
  IMMS: Intelligent Multimedia Management System
  Copyright (C) 2001-2009 Michael Grigoriev

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
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
        int relation, acoustic;
        time_t last_played;
        bool identified;
    };

    virtual bool fetch_song_info(SongData &data);
    virtual bool parse_song_info(const SongData &data, StringPair &info);

    bool identify_playlist_item(int pos);

    int next_sid, pl_length;
};

#endif
