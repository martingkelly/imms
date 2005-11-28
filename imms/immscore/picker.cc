#include <iostream>
#include <algorithm>
#include <map>

#include <math.h>

#include "picker.h"
#include "strmanip.h"
#include "immsutil.h"

#define     SAMPLE_SIZE             100
#define     MIN_SAMPLE_SIZE         35
#define     MAX_ATTEMPTS            (SAMPLE_SIZE*2)

#define     EXP                     1.10
#define     _TICKETS(x)             pow(EXP, x) * 99.0 / pow(EXP, 100)
#define     TICKETS(x)              ROUND(_TICKETS(x) + 1)

using std::endl;
using std::cerr;
using std::map;

SongPicker::SongPicker()
    : current(0, "current"), acquired(0), winner(0, "winner")
{
    reschedule_requested = playlist_known = 0;
    reset();
}

void SongPicker::reset()
{
    candidates.clear();
    metacandidates.clear();
    acquired = attempts = 0;
    selection_ready = false;
    if (reschedule_requested)
        --reschedule_requested;
}

void SongPicker::playlist_changed(int length)
{
    playlist_known = 0;
    reset();
}

bool SongPicker::add_candidate(bool urgent)
{
    int want = urgent ? MIN_SAMPLE_SIZE : SAMPLE_SIZE;
    if (candidates.empty() && metacandidates.empty())
        get_metacandidates(want);

    if (attempts > MAX_ATTEMPTS)
        return false;
    ++attempts;

    if (acquired >= want || metacandidates.empty())
        return false;

    int position = metacandidates.back();
    metacandidates.pop_back();

    string path = ImmsDb::get_item_from_playlist(position);

    request_playlist_item(position);

    if (path == "")
        return false;

    SongData data(position, path);

    if (find(candidates.begin(), candidates.end(), data)
            != candidates.end())
        return true;

    if (fetch_song_info(data))
    {
        ++acquired;
        candidates.push_back(data);
        if (urgent && data.effective_rating > 80)
            attempts = MAX_ATTEMPTS + 1;
    }

    return true;
}

bool SongPicker::do_events()
{
    if (!playlist_known || !pl_length)
        return true;

    for (int i = 0; i < 5 && !selection_ready ; ++i)
        if (!add_candidate())
        {
            selection_ready = true;
            if (reschedule_requested)
            {
                reschedule_requested = 0;
                reset_selection();
            }
        }

    if (playlist_known == 2)
        return false;

    int pos = ImmsDb::get_unknown_playlist_item();
    if (pos < 0)
    {
        playlist_known = 2;
        return false;
    }
    identify_playlist_item(pos);
    return true;
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
    if (PlaylistDb::get_real_playlist_length() < pl_length)
        return -1;

    if (add_candidate())
        request_reschedule();
    if (candidates.size() < MIN_SAMPLE_SIZE)
        while (add_candidate(true));

    if (candidates.empty())
    {
        LOG(ERROR) << "warning: no candidates!" << endl;
        return 0;
    }

    typedef map<int, vector<SongData *> > Ratings;
    Ratings ratings;

    Candidates::iterator i;
    float max_last_played = 0;

    for (i = candidates.begin(); i != candidates.end(); ++i)
        if (i->last_played > max_last_played)
            max_last_played = i->last_played;

    for (i = candidates.begin(); i != candidates.end(); ++i)
    {
        i->effective_rating += i->relation + i->specrating + i->bpmrating;
        i->effective_rating = ROUND(i->effective_rating
                * i->last_played / max_last_played);
        int tickets = TICKETS(i->effective_rating);
        ratings[tickets].push_back(&*i);
    }

    unsigned total = 0;
    for (Ratings::iterator i = ratings.begin(); i != ratings.end(); ++i)
        total += i->first;

    int winning_ticket = imms_random(total);

#ifdef DEBUG
    cerr << string(80, '-') << endl;
    cerr << " >>> ";
#endif
    for (Ratings::iterator i = ratings.begin(); i != ratings.end(); ++i)
    {
#ifdef DEBUG
        cerr << i->first << ":" << i->second.size();
#endif
        winning_ticket -= i->first;
        if (winning_ticket < 0)
        {
#ifdef DEBUG
            cerr << "* ";
#endif
            winner = *i->second[imms_random(i->second.size())];
#ifndef DEBUG
            break;
        }
#else
            winning_ticket = INT_MAX;
        }
        else
            cerr << " ";
#endif
    }
#ifdef DEBUG
    cerr << endl;
#endif

    reset();

    next_sid = winner.get_sid();

    return winner.position;
}
