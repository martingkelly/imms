/*
 *      The XMMS plugin portion of IMMS
 */

#include <string>
#include <iostream>
#include <time.h>

#include <xmms/plugin.h>
#include <xmms/xmmsctrl.h> 

#include "immsconf.h"
#include "plugin.h"
#include "player.h"
#include "imms.h"
#include "utils.h"

using std::string;

// Local vars
static Imms *imms = NULL;
int cur_plpos, next_plpos = -1, pl_length = -1;
int last_plpos = -1, last_song_length = -1;
int good_length = 0, song_length = 0, busy = 0, just_enqueued = 0, ending = 0;
bool shuffle = false;

string cur_path = "", last_path = "";

// Extern from interface.c
extern GeneralPlugin imms_gp;
int &session = imms_gp.xmms_session;

// Wrapper that frees memory
string imms_get_playlist_item(int at)
{
    if (at > pl_length - 1)
        return "";
    char *tmp = 0;
    while (!tmp)
       tmp = xmms_remote_get_playlist_file(session, at);
    string result = tmp;
    free(tmp);
    return result;
}

void imms_setup(int use_xidle)
{
    if (imms)
        imms->setup(use_xidle);
}

void imms_init()
{
    if (!imms)
    {
        imms = new Imms();
        busy = 0;
    }
}

void imms_cleanup(void)
{
    delete imms;
    imms = 0;
}

int random_index() { return imms_random(pl_length); }

static void reset_selection()
{
    xmms_remote_playqueue_remove(session, next_plpos);
    next_plpos = -1;
}

void Player::reset_selection()
{
    ::reset_selection();
}

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

static void enqueue_next()
{
    if (just_enqueued)
    {
        --just_enqueued;
        return;
    }

    // have imms select the next song for us
    next_plpos = imms->select_next();
    xmms_remote_playqueue_add(session, next_plpos);

    just_enqueued = 2;
}

static void check_playlist()
{
    // update playlist length
    int new_pl_length = xmms_remote_get_playlist_length(session);
    if (new_pl_length != pl_length)
    {
        pl_length = new_pl_length;
        reset_selection();
        imms->playlist_changed();
    }
}

static void check_time()
{
    int cur_time = xmms_remote_get_output_time(session);

    ending += song_length - cur_time < 20 * 1000
                            ? ending < 10 : -(ending > 0);
}

void do_checks()
{
    check_playlist();

    if (!xmms_remote_is_playing(session))
    {
        imms->do_idle_events();
        return;
    }

    // have imms do it's internal processing
    imms->do_events();

    cur_plpos = xmms_remote_get_playlist_pos(session);
    
    // check if xmms is reporting the song length correctly
    song_length = xmms_remote_get_playlist_time(session, cur_plpos);
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
            xmms_remote_playqueue_remove(session, next_plpos);
            return;
        }
    }

    check_time();

    bool newshuffle = xmms_remote_is_shuffle(session);
    if (!newshuffle && shuffle)
        reset_selection();
    shuffle = newshuffle;

    if (!shuffle)
        return;

    int qlength = xmms_remote_get_playqueue_length(session);
    if (qlength > 1)
        reset_selection();
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
