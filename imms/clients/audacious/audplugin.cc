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
/*
 *      The XMMS plugin portion of IMMS
 */

#include <string>
#include <iostream>
#include <time.h>

extern "C" {
#include <audacious/plugin.h>
#include <audacious/audctrl.h>
#include <audacious/drct.h>
#include <libaudcore/audstrings.h>
#include <audacious/playlist.h>
#include <audacious/misc.h>
}

#include "immsconf.h"
#include "cplugin.h"
#include "immsutil.h"
#include "clientstub.h"

using std::string;
using std::cerr;
using std::endl;

// Local vars
int cur_plpos, next_plpos = -1, pl_length = -1,
    last_plpos = -1, last_song_length = -1;
int good_length = 0, song_length = 0,
    busy = 0, just_enqueued = 0, ending = 0;
bool shuffle = true, select_pending = false, xidle_val = false;

string cur_path = "", last_path = "";

// Wrapper that frees memory
string imms_get_playlist_item(int at)
{
    if (at > pl_length - 1)
        return "";
    char* uri = 0;
    while (!uri) uri = aud_playlist_entry_get_filename(aud_playlist_get_active (), at);
    string result = uri;
    if(uri) str_unref(uri);

    gchar* realfn = g_filename_from_uri(result.c_str(), NULL, NULL);
    char* decoded = g_filename_to_utf8(realfn ? realfn : result.c_str(),
                                       -1, NULL, NULL, NULL);
    if (decoded) result = decoded;
    free(realfn);
    free(decoded);
    return result;
}

static void player_reset_selection()
{
    gint playlist = aud_playlist_get_active();
    aud_playlist_queue_delete (playlist, aud_playlist_queue_find_entry(playlist, next_plpos), 1);
    next_plpos = -1;
}

struct FilterOps;
typedef IMMSClient<FilterOps> XMMSClient;
XMMSClient *imms = 0;

static void enqueue_next()
{
    if (select_pending)
        return;
    if (just_enqueued)
    {
        --just_enqueued;
        return;
    }

    // have imms select the next song for us
    select_pending = true;
    imms->select_next();
}

struct FilterOps
{
    static void set_next(int next)
    {
        next_plpos = next;
        aud_playlist_queue_insert(aud_playlist_get_active (), -1, next_plpos);
        select_pending = false;
        just_enqueued = 2;
    }
    static void reset_selection()
    {
        player_reset_selection();
    }
    static string get_item(int index)
    {
        return imms_get_playlist_item(index);
    }
    static int get_length()
    {
        return aud_playlist_entry_count(aud_playlist_get_active());
    }
}; 

void imms_setup(int use_xidle)
{
    xidle_val = use_xidle;
    if (imms)
        imms->setup(use_xidle);
}

void imms_init()
{
    if (!imms)
    {
        imms = new XMMSClient();
        busy = 0;
    }
}

void imms_cleanup(void)
{
    delete imms;
    imms = 0;
}

int random_index() { return imms_random(pl_length); }

static void do_song_change()
{
    bool forced = (cur_plpos != next_plpos);
    bool bad = good_length < 3 || song_length < 30*1000;

    // notify imms that the previous song has ended
    if (last_path != "")
        imms->end_song(ending, forced, bad);

    // notify imms of the next song
    imms->start_song(cur_plpos, cur_path);

    last_path = cur_path;
    ending = good_length = 0;

    if (!shuffle)
        next_plpos = (cur_plpos + 1) % pl_length;
}

static void check_playlist()
{
    // update playlist length
    int new_pl_length = aud_playlist_entry_count(aud_playlist_get_active());
    if (new_pl_length != pl_length)
    {
        pl_length = new_pl_length;
        player_reset_selection();
        imms->playlist_changed(pl_length);
    }
}

static void check_time()
{
    int cur_time = aud_drct_get_time();

    ending += song_length - cur_time < 20 * 1000
                            ? ending < 10 : -(ending > 0);
}

void do_checks()
{
    check_playlist();

    if (imms->check_connection())
    {
        select_pending = false;
        imms->setup(xidle_val);
        imms->playlist_changed(pl_length =
                aud_playlist_entry_count(aud_playlist_get_active()));
        if (aud_drct_get_playing())
        {
            last_plpos = cur_plpos = aud_playlist_get_position(aud_playlist_get_active());
            last_path = cur_path = imms_get_playlist_item(cur_plpos);
            imms->start_song(cur_plpos, cur_path);
        }
        enqueue_next();
    }

    if (!aud_drct_get_playing())
        return;

    cur_plpos = aud_playlist_get_position(aud_playlist_get_active());
    
    // check if xmms is reporting the song length correctly
    song_length = aud_playlist_entry_get_length(aud_playlist_get_active(), cur_plpos, FALSE);
    if (song_length > 1000)
        good_length++;

    if (last_plpos != cur_plpos || last_song_length != song_length)
    {
        cur_path = imms_get_playlist_item(cur_plpos);
        if (cur_path == "")
            return;

        last_song_length = song_length;
        last_plpos = cur_plpos;

        if (last_path != cur_path)
        {
            do_song_change();
            gint playlist = aud_playlist_get_active();
            aud_playlist_queue_delete(playlist, aud_playlist_queue_find_entry(playlist, next_plpos), 1);
            return;
        }
    }

    check_time();

    bool newshuffle = aud_get_bool(NULL, "shuffle");
    if (!newshuffle && shuffle)
        player_reset_selection();
    shuffle = newshuffle;

    if (!shuffle)
        return;

    int qlength = aud_playlist_queue_count(aud_playlist_get_active());
    if (qlength > 1)
        player_reset_selection();
    else if (!qlength)
        enqueue_next();
}

void imms_poll()
{
    if (busy)
        return;

    ++busy;
    do_checks();
    --busy;
}
