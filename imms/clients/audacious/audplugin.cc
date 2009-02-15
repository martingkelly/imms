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

#include <audacious/plugin.h>
#include <audacious/audctrl.h>

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

extern "C" {
void init(void);
void about(void);
void configure(void);
void cleanup(void);
}

static GeneralPlugin imms_gp =
{
    0,
    0,
    PACKAGE_STRING,
    init,
    cleanup,
    about,
    configure
};

GeneralPlugin *gp_plugin_list[] = { &imms_gp, NULL };

SIMPLE_GENERAL_PLUGIN(imms, gp_plugin_list);

// Wrapper that frees memory
string imms_get_playlist_item(int at)
{
    if (at > pl_length - 1)
        return "";
    char *tmp = 0;
    while (!tmp)
       tmp = audacious_drct_get_playlist_file(at);
    string result = tmp;
    free(tmp);
    gchar *realfn = g_filename_from_uri(result.c_str(), NULL, NULL);
    tmp = aud_filename_to_utf8(realfn ? realfn : result.c_str());
    result = tmp;
    free(tmp);
    free(realfn);
    return result;
}

static void player_reset_selection()
{
    audacious_drct_playqueue_remove(next_plpos);
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
        audacious_drct_playqueue_add(next_plpos);
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
        return audacious_drct_get_playlist_length();
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
    int new_pl_length = audacious_drct_get_playlist_length();
    if (new_pl_length != pl_length)
    {
        pl_length = new_pl_length;
        player_reset_selection();
        imms->playlist_changed(pl_length);
    }
}

static void check_time()
{
    int cur_time = audacious_drct_get_output_time();

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
                audacious_drct_get_playlist_length());
        if (audacious_drct_is_playing())
        {
            last_plpos = cur_plpos = audacious_drct_get_playlist_pos();
            last_path = cur_path = imms_get_playlist_item(cur_plpos);
            imms->start_song(cur_plpos, cur_path);
        }
        enqueue_next();
    }

    if (!audacious_drct_is_playing())
        return;

    cur_plpos = audacious_drct_get_playlist_pos();
    
    // check if xmms is reporting the song length correctly
    song_length = audacious_drct_get_playlist_time(cur_plpos);
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
            audacious_drct_playqueue_remove(next_plpos);
            return;
        }
    }

    check_time();

    bool newshuffle = audacious_drct_is_shuffle();
    if (!newshuffle && shuffle)
        player_reset_selection();
    shuffle = newshuffle;

    if (!shuffle)
        return;

    int qlength = audacious_drct_get_playqueue_length();
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
