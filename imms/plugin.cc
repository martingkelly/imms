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
#include "imms.h"

using std::string;
using std::cerr;

#define DEFAULT_EMAIL       "default@imms.org"
#define POLL_DELAY          5

// Local vars
static Imms *imms = NULL;
unsigned int time_left = 1000, sloppy_skips = 0;
int last_plpos = -2, cur_plpos, pl_length = -1;
int good_length = 0, song_length = 0, delay = 0;
string cur_path = "", last_path = "";
bool need_more = true, spectrum_ok = false;

// Extern from interface.c
extern VisPlugin imms_vp;
int &session = imms_vp.xmms_session;

static enum
{
    IDLE        = 0,
    BUSY        = 1,
    FIND_NEXT   = 2
} state;

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

void imms_setup(char *ch_email, int use_xidle, int use_sloppy)
{
    sloppy_skips = use_sloppy;

    if (imms)
        imms->setup(ch_email ? ch_email : DEFAULT_EMAIL, use_xidle);
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

void imms_spectrum(uint16_t spectrum[256])
{
    if (imms && spectrum_ok)
        imms->integrate_spectrum(spectrum);
}

void do_more_checks()
{
    delay = 0;

    // make sure shuffle is disabled
    if (xmms_remote_is_shuffle(session))
        xmms_remote_toggle_shuffle(session);

    // update playlist length
    int new_pl_length = xmms_remote_get_playlist_length(session);
    if (new_pl_length != pl_length)
    {
        pl_length = new_pl_length;
        imms->playlist_changed(pl_length);
    }

    // check if xmms is reporting the song length correctly
    song_length = xmms_remote_get_playlist_time(session, cur_plpos);
    if (song_length > 1000)
        good_length++;

    // have imms do it's internal processing
    imms->pump();
}

void do_spectrum_checks(int cur_time)
{
    const static float skip = 0.15;

    bool _spectrum_ok = (cur_time > song_length * skip
            && cur_time < song_length * (1 - skip));

    if (spectrum_ok && !_spectrum_ok)
        imms->stop_spectrum_analysis();

    if (!spectrum_ok && _spectrum_ok)
        imms->start_spectrum_analysis();

    spectrum_ok = _spectrum_ok;
}

void do_checks()
{
    // if not playing make sure we stopped collecting spectrum statistics
    if (!xmms_remote_is_playing(session))
    {
        if (spectrum_ok)
            imms->stop_spectrum_analysis();
        spectrum_ok = false;
        return;
    }

    // run these checks less frequently so as not to waste cpu time
    if (++delay > POLL_DELAY || pl_length < 0 || good_length < 3)
        do_more_checks();

    bool ending = good_length > 2 && time_left == 0;
    cur_plpos = xmms_remote_get_playlist_pos(session);

    if (ending || cur_plpos != last_plpos)
    {
        cur_path = imms_get_playlist_item(cur_plpos);

        if (ending || last_path != cur_path)
        {
            xmms_remote_stop(session);
            if (last_path == cur_path)
                xmms_remote_playlist_next(session);

            state = FIND_NEXT;
            return;
        }

        last_plpos = cur_plpos;
    }

    // check the time to catch the end of the song
    int cur_time = xmms_remote_get_output_time(session);
    time_left = (song_length - cur_time) / 500;

    do_spectrum_checks(cur_time);

    // if we don't have enough, feed imms more candidates for the next song
    if (need_more && delay % 2)
    {
        int pos = imms_random(xmms_remote_get_playlist_length(session));
        need_more = imms->add_candidate(pos, imms_get_playlist_item(pos));
    }
}

void do_find_next()
{
    if (time_left < 8 * (sloppy_skips + 1) * 2)
        time_left = 0;

    if (last_plpos == -2)
        last_plpos = cur_plpos - 1;

    cur_plpos = xmms_remote_get_playlist_pos(session);
    bool forced = ((last_plpos + 1) % pl_length ) != cur_plpos;
    bool bad = good_length < 3 || song_length <= 30*1000;

    // notify imms that the previous song has ended
    if (last_path != "")
        imms->end_song(!time_left, forced, bad);

    // if the song was not directly picked by the user...
    if (!forced && pl_length > 2)
    {
        if (need_more)
            do { cur_plpos = imms_random(pl_length); }
            while (imms->add_candidate(cur_plpos,
                        imms_get_playlist_item(cur_plpos), true));

        // have imms select the next song for us
        cur_plpos = imms->select_next();
        cur_path = imms_get_playlist_item(cur_plpos);
        xmms_remote_set_playlist_pos(session, cur_plpos);
    }

    // notify imms of the next song
    imms->start_song(cur_path);

    last_path = cur_path;
    good_length = 0;
    need_more = true;

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
