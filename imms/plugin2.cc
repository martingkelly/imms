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

#define DEFAULT_EMAIL       "default@imms.org"
#define SPECTRUM_SKIP       0.15
#define POLL_DELAY          3

// Local vars
static Imms *imms = NULL;
int cur_plpos, next_plpos = -1, pl_length = -1, last_time = -1;
int good_length = 0, song_length = 0, busy = 0, just_enqueued = 0, ending = 0;
bool spectrum_ok = false, shuffle = false;

string cur_path = "", last_path = "";

// Extern from interface.c
extern VisPlugin imms_vp;
int &session = imms_vp.xmms_session;

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

void imms_setup(char *ch_email, int use_xidle)
{
    if (imms)
        imms->setup(ch_email ? ch_email : DEFAULT_EMAIL, use_xidle);
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

void imms_spectrum(uint16_t spectrum[256])
{
    if (imms && spectrum_ok)
        imms->integrate_spectrum(spectrum);
}

int random_index() { return imms_random(pl_length); }

void reset_selection()
{
    xmms_remote_playqueue_remove(session, next_plpos);
    next_plpos = -1;
}

void check_playlist()
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

void do_more_checks()
{
    // run these checks less frequently so as not to waste cpu time
    static int delay = 0;
    if (++delay < POLL_DELAY && pl_length > -1 && good_length > 2)
        return;
    delay = 0;

    // check if xmms is reporting the song length correctly
    song_length = xmms_remote_get_playlist_time(session, cur_plpos);
    if (song_length > 1000)
        good_length++;

    bool newshuffle = xmms_remote_is_shuffle(session);
    if (!newshuffle && shuffle)
        reset_selection();
    shuffle = newshuffle;
}

void do_song_change()
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
}

void enqueue_next()
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

void do_checks()
{
    // if not playing do nothing
    if (!xmms_remote_is_playing(session))
        return;

    check_playlist();

    // check the time to catch the end of the song
    int cur_time = xmms_remote_get_output_time(session);

    if (last_time != cur_time)
    {
        cur_plpos = xmms_remote_get_playlist_pos(session);
        cur_path = imms_get_playlist_item(cur_plpos);
        if (cur_path == "")
            return;

        last_time = cur_time;

        if (last_path != cur_path)
        {
            do_song_change();
            xmms_remote_playqueue_remove(session, next_plpos);
            return;
        }
    }

    spectrum_ok = (cur_time > song_length * SPECTRUM_SKIP
            && cur_time < song_length * (1 - SPECTRUM_SKIP));

    ending += song_length - cur_time < 20 * 1000
                            ? ending < 10 : -(ending > 0);

    do_more_checks();

    // have imms do it's internal processing
    imms->pump();

    int qlength = xmms_remote_get_playqueue_length(session);
    if (shuffle)
    {
        if (qlength > 1)
        {
            xmms_remote_playqueue_remove(session, next_plpos);
            next_plpos = -1;
        }
        else if (!qlength)
        {
            enqueue_next();
            return;
        }
    }
    else
    {
        cur_plpos = xmms_remote_get_playlist_pos(session);
        next_plpos = (cur_plpos + 1) % pl_length;
    }
}

void imms_poll()
{
    if (busy)
        return;

    ++busy;
    do_checks();
    --busy;
}
