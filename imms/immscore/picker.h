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
#ifndef __PICKER_H
#define __PICKER_H

#include <string>
#include <list>
#include <vector>

#include "immsconf.h"
#include "fetcher.h"

class SongPicker : protected InfoFetcher
{
public:
    SongPicker();
    virtual int select_next();
    virtual void playlist_ready() { playlist_known = 1; }
    virtual void playlist_changed(int length);

    void request_reschedule() { reschedule_requested = 2; }

protected:
    bool add_candidate(bool urgent = false);
    void revalidate_current(int pos, const std::string &path);
    bool do_events();
    void reset();

    // To be implemented in Imms
    virtual void reset_selection() = 0;
    virtual void request_playlist_item(int index) = 0;
    virtual void get_metacandidates(int size) = 0;

    SongData current;
    std::vector<int> metacandidates;

private:
    void get_related(int pivot_sid, int limit);

    bool selection_ready;
    int reschedule_requested;
    int acquired, attempts, playlist_known;
    SongData winner;

    typedef std::list<SongData> Candidates;
    Candidates candidates;
};

#endif
