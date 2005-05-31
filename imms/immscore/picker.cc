#include <iostream>
#include <algorithm>

#include <math.h>

#include "picker.h"
#include "strmanip.h"
#include "immsutil.h"

#define     SAMPLE_SIZE             100
#define     MIN_SAMPLE_SIZE         35
#define     MAX_ATTEMPTS            (SAMPLE_SIZE*2)
#define     TICKETS(x)              ROUND(pow(2.5, x/20) * 100 / pow(2.5, 5))

using std::endl;
using std::cerr;

SongPicker::SongPicker()
    : current(0, "current"), acquired(0), winner(0, "winner")
{
    playlist_known = 0;
    reset();
}

void SongPicker::reset()
{
    candidates.clear();
    metacandidates.clear();
    acquired = attempts = 0;
}

void SongPicker::playlist_changed(int length)
{
    playlist_known = 0;
    reset();
}

int SongPicker::next_candidate()
{
    int pos = -1;
    if (!metacandidates.empty())
    {
        pos = metacandidates.back();
        metacandidates.pop_back();
        return pos;
    }

    while (1)
    {
        int pos = ImmsDb::random_playlist_position();
        if (pos < 0)
            return -1;

        bool found = false;
        for (Candidates::iterator i = candidates.begin();
                !found && i != candidates.end(); ++i)
        {
            if (i->position == pos)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return pos;
    }
}

bool SongPicker::add_candidate(bool urgent)
{
    if (candidates.empty() && metacandidates.empty())
        get_metacandidates();

    if (attempts > MAX_ATTEMPTS)
        return false;
    ++attempts;

    int want = urgent ? MIN_SAMPLE_SIZE : SAMPLE_SIZE;
    if (acquired >= std::min(want, pl_length))
        return false;

    int position = next_candidate();

    if (position == -1)
        return false;

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
        if (urgent && data.tickets > 80)
            attempts = MAX_ATTEMPTS + 1;
    }

    return true;
}

bool SongPicker::do_events()
{
    if (!playlist_known || !pl_length)
        return true;

    for (int i = 0; i < 5 && add_candidate(); ++i);

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
    if (PlaylistDb::get_effective_playlist_length(true) < pl_length)
        return -1;

    if (candidates.size() < MIN_SAMPLE_SIZE)
        while (add_candidate(true));

    if (candidates.empty())
    {
        cerr << "IMMS: warning: no candidates!" << endl;
        return 0;
    }

    Candidates::iterator i;
    unsigned total = 0;
    float max_last_played = 0;

    for (i = candidates.begin(); i != candidates.end(); ++i)
        if (i->last_played > max_last_played)
            max_last_played = i->last_played;

    for (i = candidates.begin(); i != candidates.end(); ++i)
    {
        i->tickets = TICKETS(i->effective_rating) + i->relation +
            i->specrating + i->bpmrating;

        i->tickets = ROUND(i->tickets * i->last_played / max_last_played);
        total += i->tickets;
    }

#ifdef DEBUG
    cerr << string(80 - 15, '-') << endl;
    cerr << " >> ";
#endif

    int winning_ticket = imms_random(total);

    for (i = candidates.begin(); i != candidates.end(); ++i)
    {
        winning_ticket -= i->tickets;
        if (winning_ticket < 0)
        {
            winner = *i;
#ifndef DEBUG
            break;
        }
    }
#else
            winning_ticket = INT_MAX;
            cerr << "{" << i->tickets << "} ";
        }
        else if (i->tickets)
            cerr << i->tickets << " ";
    }
    cerr << endl;
#endif

    reset();

    next_sid = winner.get_sid();

    return winner.position;
}
