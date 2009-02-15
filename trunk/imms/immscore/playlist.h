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
#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include "immsconf.h"
#include "basicdb.h"
#include "song.h"

#include <vector>

class PlaylistDb
{
public:
    PlaylistDb() : effective_length_cache(-1) { clear_matches(); }
    virtual ~PlaylistDb() {};
    void playlist_insert_item(int pos, const string &path);
    void playlist_update_identity(int pos, int uid);
    static Song playlist_id_from_item(int pos);

    string get_item_from_playlist(int pos);
    int get_unknown_playlist_item();

    int get_real_playlist_length();
    int get_effective_playlist_length();
    void get_random_sample(std::vector<int> &metacandidates, int size);

    void playlist_clear();
    void playlist_ready()
    {
        sync();
        playlist_updated();
    }
    void sync();
    void clear_matches();

    virtual void playlist_updated() {};

protected:
    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0) {}

private:
    int effective_length_cache;
};

#endif
