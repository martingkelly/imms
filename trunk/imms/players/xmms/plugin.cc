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

using std::string;

#define POLL_DELAY          5

// Local vars
static Imms *imms = NULL;
unsigned int time_left = 1000, sloppy_skips = 0;
int last_plpos = -2, cur_plpos, pl_length = -1;
int good_length = 0, song_length = 0, delay = 0;
string cur_path = "", last_path = "";

// Extern from interface.c
extern GeneralPlugin imms_gp;
int &session = imms_gp.xmms_session;

static enum
{
    IDLE        = 0,
    BUSY        = 1,
    FIND_NEXT   = 2
} state;

void Player::reset_selection() {}

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
        imms = new Imms();

    state = IDLE;
}

void imms_cleanup(void)
{
    delete imms;
    imms = 0;
}

void do_more_checks()
{
    delay = 0;

    // make sure shuffle is disabled
    if (imms && xmms_remote_is_shuffle(session))
        xmms_remote_toggle_shuffle(session);

    // update playlist length
    int new_pl_length = xmms_remote_get_playlist_length(session);
    if (new_pl_length != pl_length)
    {
        pl_length = new_pl_length;
        imms->playlist_changed();
    }

    // check if xmms is reporting the song length correctly
    song_length = xmms_remote_get_playlist_time(session, cur_plpos);
    if (song_length > 1000)
        good_length++;

    // have imms do it's internal processing
    imms->do_events();
}

void do_checks()
{
    if (last_plpos == -2)
        last_plpos = xmms_remote_get_playlist_pos(session) - 1;

    if (!xmms_remote_is_playing(session))
    {
        imms->do_idle_events();
        return;
    }

    // run these checks less frequently so as not to waste cpu time
    if (++delay > POLL_DELAY || pl_length < 0 || good_length < 3)
        do_more_checks();

    // do not preemptively end the song if imms is disabled 
    // to allow the built in shuffle/sequential to take effect
    bool ending = good_length > 2 && time_left == 0;
    cur_plpos = xmms_remote_get_playlist_pos(session);

    if (ending || cur_plpos != last_plpos)
    {
        cur_path = imms_get_playlist_item(cur_plpos);

        if (ending || last_path != cur_path)
        {
            xmms_remote_stop(session);
            state = FIND_NEXT;
            return;
        }

        last_plpos = cur_plpos;
    }

    // check the time to catch the end of the song
    int cur_time = xmms_remote_get_output_time(session);
    if (cur_time > 1000 || good_length < 3)
        time_left = (song_length - cur_time) / 500;
}

void do_find_next()
{
    if (time_left < 20)
        time_left = 0;

    cur_plpos = xmms_remote_get_playlist_pos(session);
    bool forced = (cur_plpos != last_plpos) && 
        ((last_plpos + 1) % pl_length) != cur_plpos;
    bool back = ((last_plpos + pl_length - 1) % pl_length) == cur_plpos;
    bool bad = good_length < 3 || song_length <= 30*1000;

    // notify imms that the previous song has ended
    if (last_path != "")
        imms->end_song(!time_left, forced, bad);

    if (!forced && pl_length > 2)
    {
        // have imms select the next song for us
        cur_plpos = imms->select_next();
    }
    else if (back)
    {
        int previous = imms->get_previous();
        if (previous != -1)
            cur_plpos = previous;
    }

    cur_path = imms_get_playlist_item(cur_plpos);
    xmms_remote_set_playlist_pos(session, cur_plpos);

    // notify imms of the next song
    imms->start_song(cur_plpos, cur_path);

    last_path = cur_path;
    good_length = 0;

    xmms_remote_play(session);
}

void imms_poll()
{
    switch (state)
    {
    case BUSY:
        return;

    case IDLE:
        state = BUSY;
        do_checks();
        if (state == BUSY)
            state = IDLE;
        return;

    case FIND_NEXT:
        state = BUSY;
        do_find_next();
        state = IDLE;
        return;
    }
}
