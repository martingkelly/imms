#include <iostream>
#include <algorithm>

#include <math.h>

#include "picker.h"
#include "strmanip.h"
#include "player.h"
#include "utils.h"

#define     SAMPLE_SIZE             100
#define     MIN_SAMPLE_SIZE         35
#define     REALLY_GOOD             50000
#define     MAX_ATTEMPTS            (SAMPLE_SIZE*2)
#define     BASE_BIAS               10
#define     DISPERSION_FACTOR       4.1

using std::endl;
using std::cerr;

SongPicker::SongPicker()
{
    reset();
}

void SongPicker::reset()
{
    candidates.clear();
    acquired = attempts = 0;
}

bool SongPicker::add_candidate(bool urgent)
{
    if (attempts > MAX_ATTEMPTS)
        return false;
    ++attempts;

    int want = urgent ? MIN_SAMPLE_SIZE : SAMPLE_SIZE;
    if (acquired >= std::min(want, Player::get_playlist_length()))
        return false;

    int position = ImmsDb::random_playlist_position();
    if (position < 0)
        position = imms_random(Player::get_playlist_length());
    string path = ImmsDb::get_playlist_item(position);
    string realpath = path_normalize(Player::get_playlist_item(position));

    if (path != realpath)
    {
        cerr << "IMMS: path triggered playlist refresh" << endl;
        cerr << path << " != " << realpath << endl;
        playlist_changed();
        return true;
    }

    SongData data(position, path);

    if (find(candidates.begin(), candidates.end(), data)
            != candidates.end())
        return true;

    if (fetch_song_info(data))
    {
        ++acquired;
        candidates.push_back(data);
        if (urgent && data.composite_rating > REALLY_GOOD)
            attempts = MAX_ATTEMPTS + 1;
    }

    return true;
}

void SongPicker::identify_more()
{
    if (playlist_ready)
        return;
    int pos = ImmsDb::get_unknown_playlist_item();
    if (pos < 0)
    {
        playlist_ready = true;
        return;
    }
    playlist_identify_item(pos);
}

void SongPicker::do_events()
{
    bool more = false;
    for (int i = 0; i < 4 && (more = add_candidate()); ++i);
    if (!more)
        identify_more();
}

void SongPicker::revalidate_current(int pos, const string &path)
{
    if (winner.position == pos && winner.get_path() == path)
    {
        current = winner;
    }
    else if (current.get_path() != path || current.position != pos)
    {
        current = SongData(pos, path);
        fetch_song_info(current);
    }
}

int SongPicker::select_next()
{
    if (candidates.size() < MIN_SAMPLE_SIZE)
        while (add_candidate(true));

    if (candidates.empty())
    {
        cerr << "IMMS: warning: no candidates!" << endl;
        return 0;
    }

    Candidates::iterator i;
    unsigned int total = 0;
    time_t max_last_played = 0;
    int max_composite = -INT_MAX, min_composite = INT_MAX;

    for (i = candidates.begin(); i != candidates.end(); ++i)
        if (i->last_played > max_last_played)
            max_last_played = i->last_played;

    for (i = candidates.begin(); i != candidates.end(); ++i)
    {
        i->composite_rating =
            ROUND((i->rating + i->relation + i->specrating + i->bpmrating)
                * i->last_played / (double)max_last_played);
        
        if (i->composite_rating > max_composite)
            max_composite = i->composite_rating;
        if (i->composite_rating < min_composite)
            min_composite = i->composite_rating;
    }

    bool have_good = (max_composite > MIN_RATING);
    if (have_good && min_composite < MIN_RATING)
        min_composite = MIN_RATING;

    for (i = candidates.begin(); i != candidates.end(); ++i)
    {
        if (have_good && i->composite_rating < MIN_RATING)
        {
            i->composite_rating = 0;
            continue;
        }

        i->composite_rating =
            ROUND(pow(((double)(i->composite_rating - min_composite))
                        / DISPERSION_FACTOR, DISPERSION_FACTOR));
        i->composite_rating += BASE_BIAS;
        total += i->composite_rating;
    }
#ifdef DEBUG
    cerr << string(80 - 15, '-') << endl;
    cerr << " >> [" << min_composite << "-" << max_composite << "] ";
#endif

    int weight_index = imms_random(total);

    for (i = candidates.begin(); i != candidates.end(); ++i)
    {
        weight_index -= i->composite_rating;
        if (weight_index < 0)
        {
            winner = *i;
#ifndef DEBUG
            break;
        }
    }
#else
            weight_index = INT_MAX;
            cerr << "{" << i->composite_rating << "} ";
        }
        else if (i->composite_rating)
            cerr << i->composite_rating << " ";
    }
    cerr << endl;
#endif

    reset();

    next_sid = winner.get_sid();

    return winner.position;
}
