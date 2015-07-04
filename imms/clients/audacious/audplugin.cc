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

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/mainloop.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudcore/runtime.h>

#include "immsconf.h"
#include "immsutil.h"
#include "clientstub.h"

#define USE_XIDLE true

using std::string;

class IMMSPlugin : public GeneralPlugin
{
public:
    static const char about[];

    static constexpr PluginInfo info = {
        "IMMS",
        nullptr,
        about
    };

    constexpr IMMSPlugin() : GeneralPlugin(info, false) {}

    bool init();
    void cleanup();
};

IMMSPlugin aud_plugin_instance;

const char IMMSPlugin::about[] =
    "Intelligent Multimedia Management System\n\n"
    "IMMS is an intelligent playlist plugin for Audacious that tracks your "
    "listening patterns and dynamically adapts to your taste.\n\n"
    "It is incredibly unobtrusive and easy to use as it requires no direct "
    "user interaction.\n\n"
    "For more information please visit:\n"
    "http://www.luminal.org/wiki/index.php/IMMS\n\n"
    "Written by Michael \"mag\" Grigoriev <mag@luminal.org>";

// Local vars
static QueuedFunc timer;

int cur_plpos, next_plpos = -1, pl_length = -1,
    last_plpos = -1, last_song_length = -1;
int good_length = 0, song_length = 0,
    just_enqueued = 0, ending = 0;
bool shuffle = true, select_pending = false;

string cur_path = "", last_path = "";

static void do_checks(void *);

// Wrapper that frees memory
string imms_get_playlist_item(int at)
{
    int pl = aud_playlist_get_playing();
    String uri = aud_playlist_entry_get_filename(pl, at);
    StringBuf fn = uri ? uri_to_filename(uri, false) : StringBuf();
    return fn ? string(fn) : "";
}

static void player_reset_selection()
{
    int pl = aud_playlist_get_playing();
    int qp = aud_playlist_queue_find_entry(pl, next_plpos);
    aud_playlist_queue_delete(pl, qp, 1);
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
        int pl = aud_playlist_get_playing();
        aud_playlist_queue_insert(pl, -1, next_plpos);
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
        int pl = aud_playlist_get_playing();
        return aud_playlist_entry_count(pl);
    }
};

bool IMMSPlugin::init()
{
    imms = new XMMSClient();
    imms->setup(USE_XIDLE);
    timer.start(200, do_checks, nullptr);
    return true;
}

void IMMSPlugin::cleanup()
{
    timer.stop();
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

static void check_playlist(int pl)
{
    // update playlist length
    int new_pl_length = aud_playlist_entry_count(pl);
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

static void do_checks(void *)
{
    int pl = aud_playlist_get_playing();
    check_playlist(pl);

    if (imms->check_connection())
    {
        select_pending = false;
        imms->setup(USE_XIDLE);
        pl_length = aud_playlist_entry_count(pl);
        imms->playlist_changed(pl_length);
        if (aud_drct_get_playing())
        {
            last_plpos = cur_plpos = aud_playlist_get_position(pl);
            last_path = cur_path = imms_get_playlist_item(cur_plpos);
            imms->start_song(cur_plpos, cur_path);
        }
        enqueue_next();
    }

    if (!aud_drct_get_playing())
        return;

    cur_plpos = aud_playlist_get_position(pl);

    // check if xmms is reporting the song length correctly
    Tuple tu = aud_playlist_entry_get_tuple(pl, cur_plpos);
    song_length = tu.get_int(Tuple::Length);
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
            int qp = aud_playlist_queue_find_entry(pl, next_plpos);
            aud_playlist_queue_delete(pl, qp, 1);
            return;
        }
    }

    check_time();

    bool newshuffle = aud_get_bool(nullptr, "shuffle");
    if (!newshuffle && shuffle)
        player_reset_selection();
    shuffle = newshuffle;

    if (!shuffle)
        return;

    int qlength = aud_playlist_queue_count(pl);
    if (qlength > 1)
        player_reset_selection();
    else if (!qlength)
        enqueue_next();
}
