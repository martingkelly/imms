#include <iostream>

#include "picker.h"
#include "strmanip.h"

#define     SAMPLE_SIZE             80
#define     MIN_SAMPLE_SIZE         30
#define     MAX_ATTEMPTS            (SAMPLE_SIZE*2)
#define     BASE_BIAS               10
#define     DISPERSION_FACTOR       4.0
#define     CORRELATION_FACTOR      3

using std::endl;
using std::cerr;

SongPicker::SongPicker()
{
    reset();
}

void SongPicker::reset()
{
    candidates.clear();
    have_candidates = attempts = 0;
}

bool SongPicker::add_candidate(int playlist_num, string path, bool urgent)
{
    ++attempts;
    if (attempts > MAX_ATTEMPTS)
        return false;

    SongData data(playlist_num, path);

    if (find(candidates.begin(), candidates.end(), data)
            != candidates.end())
        return true;

    if (!fetch_song_info(data))
    {
        // In case the playlist just a has a lot of songs that are not
        // currently accessible, don't count a failure to fetch info about
        // it as an attempt, unless we need to select the next song quickly
        attempts -= !urgent;
        return true;
    }

    ++have_candidates;
    candidates.push_back(data);

    return have_candidates < (urgent ? MIN_SAMPLE_SIZE : SAMPLE_SIZE);
}

void SongPicker::revalidate_winner(const string &path)
{
    string simple = path_simplifyer(path);
    if (winner.path != simple)
    {
        winner.path = simple;
        fetch_song_info(winner);
    }
}

int SongPicker::select_next()
{
    if (candidates.empty())
        return 0;

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
            ROUND((i->rating + i->relation * CORRELATION_FACTOR)
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
#ifdef DEBUG
            weight_index = INT_MAX;
            cerr << "{" << i->composite_rating << "} ";
#else
            break;
#endif
        }
#ifdef DEBUG
        else if (i->composite_rating)
            cerr << i->composite_rating << " ";
#endif
    }

#ifdef DEBUG
    cerr << endl;
#endif

    return winner.position;
}
