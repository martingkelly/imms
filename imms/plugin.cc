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
int last_plpos = -2, cur_plpos, last_pl_length = -1;
int good_length = 0, song_length = 0, delay = POLL_DELAY;
string cur_path = "", last_path = "";
bool need_more = false;

// Extern from interface.c
extern VisPlugin imms_vp;
int &session = imms_vp.xmms_session;
extern int spectrum_ok;

static enum
{
    IDLE        = 0,
    BUSY        = 1,
    FIND_NEXT   = 2
} state;

inline int imms_get_playlist_length()
{
    return xmms_remote_get_playlist_length(session);
}

// Wrapper that frees memory
string imms_get_playlist_item(int at)
{
    if (at > last_pl_length - 1)
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
    if (imms)
        imms->integrate_spectrum(spectrum);
}

void imms_poll()
{
    if (--delay < 0)
        delay = POLL_DELAY;

    switch (state)
    {
        case BUSY:
            return;

        case IDLE:
            if (last_plpos == -2)
                last_plpos = xmms_remote_get_playlist_pos(session) - 1;

            if (!xmms_remote_is_playing(session))
                return;

            state = BUSY;

            if (last_pl_length < 0 || !delay)
            {
                if (xmms_remote_is_shuffle(session))
                    xmms_remote_toggle_shuffle(session);

                int pl_length = imms_get_playlist_length();
                if (pl_length != last_pl_length)
                {
                    last_pl_length = pl_length;
                    imms->playlist_changed(pl_length);
                }
            }

            cur_plpos = xmms_remote_get_playlist_pos(session);
            cur_path = imms_get_playlist_item(cur_plpos);

            if (last_path != cur_path || (good_length > 2 && time_left == 0))
            {
                xmms_remote_stop(session);
                if (last_path == cur_path)
                    xmms_remote_playlist_next(session);

                state = FIND_NEXT;
                return;
            }

            if (good_length < 3 || !delay)
            {
                song_length = xmms_remote_get_playlist_time(session,
                        cur_plpos);
                if (song_length > 1000)
                    good_length++;
            }

            {
                int cur_time = xmms_remote_get_output_time(session);
                time_left = (song_length - cur_time) / 500;

                last_plpos = cur_plpos;

                spectrum_ok = (cur_time > song_length * 0.1
                        && cur_time < song_length * 0.9);
            }

            if (need_more && delay % 2)
            {
                int pos = imms_random(imms_get_playlist_length());
                need_more = imms->add_candidate(pos,
                        imms_get_playlist_item(pos));
            }

            if (!delay)
                imms->pump();

            state = IDLE;
            return;

        case FIND_NEXT:
            state = BUSY;

            if (time_left < 8 * (sloppy_skips + 1) * 2)
                time_left = 0;

            int pl_length = imms_get_playlist_length();
            cur_plpos = xmms_remote_get_playlist_pos(session);
            bool forced = (last_plpos + 1 != cur_plpos) &&
                (cur_plpos != 0 || last_plpos != pl_length - 1);

            bool bad = good_length < 3 || song_length <= 30*1000;

            if (last_path != "")
                imms->end_song(!time_left, forced, bad);

            if (!forced && pl_length > 2)
            {
                do { cur_plpos = imms_random(pl_length); }
                while (imms->add_candidate(cur_plpos,
                            imms_get_playlist_item(cur_plpos), true));

                cur_plpos = imms->select_next();
                cur_path = imms_get_playlist_item(cur_plpos);

                xmms_remote_set_playlist_pos(session, cur_plpos);
            }

            imms->start_song(cur_path);

            last_path = cur_path;
            good_length = 0;
            need_more = true;

            xmms_remote_play(session);

            state = IDLE;
            return;
    }
}
