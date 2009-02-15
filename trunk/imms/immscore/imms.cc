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
#include <time.h>
#include <ctype.h>
#include <math.h>

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "imms.h"
#include "flags.h"
#include "strmanip.h"
#include "immsutil.h"

#include <model/distance.h>

using std::string;
using std::endl;
using std::cerr;
using std::setprecision;
using std::ofstream;

//////////////////////////////////////////////
// Constants

#define     MAX_TIME                21*DAY

#define     MAX_CORRELATION         12.0
#define     CORRELATION_IMPACT      40

#define     LAST_EXPIRE             HOUR

#define     TERM_WIDTH              80

#define     ACOUSTIC_IMPACT         40

//////////////////////////////////////////////

// Imms
Imms::Imms(IMMSServer *server) : server(server)
{
    last_skipped = last_jumped = false;
    local_max = MAX_TIME;

    handpicked.set_on = 0;
    last.sid = handpicked.sid = -1;

    fout.open(get_imms_root().append("imms.log").c_str(),
            ofstream::out | ofstream::app);

    time_t t = time(0);
    fout << endl << endl << ctime(&t) << setprecision(3);
}

Imms::~Imms()
{
    clear_recent();
}

void Imms::setup(bool use_xidle)
{
    xidle_enabled = use_xidle;
}

void Imms::get_metacandidates(int size)
{
    metacandidates.clear();

    if (handpicked.sid != -1)
        CorrelationDb::get_related(metacandidates, handpicked.sid, 30);
    if (last.sid != -1)
        CorrelationDb::get_related(metacandidates, last.sid, 20);

    if ((int)metacandidates.size() < size)
        PlaylistDb::get_random_sample(metacandidates,
                size - metacandidates.size());

    reverse(metacandidates.begin(), metacandidates.end());
}

void Imms::do_events()
{
    if (!SongPicker::do_events())
        CorrelationDb::maybe_expire_recent();
    XIdle::query();
}

void Imms::request_playlist_item(int index)
{
    return server->request_playlist_item(index);
}

void Imms::playlist_changed(int length)
{
    pl_length = length;
    local_max = std::min(MAX_TIME, pl_length * 8 * 60);

    ImmsDb::clear_recent();
    PlaylistDb::playlist_clear();
    SongPicker::playlist_changed(length);
} 

void Imms::playlist_ready()
{
    PlaylistDb::playlist_ready();
    SongPicker::playlist_ready();
}

void Imms::sync(bool incharge)
{
    if (!incharge)
        PlaylistDb::clear_matches();
    PlaylistDb::sync();
    SongPicker::reset();
    local_max = std::min(MAX_TIME,
            ImmsDb::get_effective_playlist_length() * 8 * 60);
    reset_selection();
}

void Imms::reset_selection()
{
    server->reset_selection();
}

void Imms::start_song(int position, string path)
{
    XIdle::reset();

    path = path_normalize(path);
    revalidate_current(position, path);

    try {
        AutoTransaction at;

        current.set_last(time(0));

        print_song_info();

        if (last_jumped)
        {
            set_lastinfo(handpicked);
            SongPicker::request_reschedule();
        }

        at.commit();
    } 
    WARNIFFAILED();

#ifdef ANALYZER_ENABLED
    if (!current.isanalyzed())
    {
        string epath = rex.replace(path, "'", "'\"'\"'", Regexx::global);
        system(string("analyzer '" + epath + "' &").c_str());
    }
#endif
}

void Imms::print_song_info()
{
    fout << string(TERM_WIDTH - 15, '-') << endl << "[";

    const string &path = current.get_path();
    if (path.length() > TERM_WIDTH - 2)
        fout << "..." << path.substr(path.length() - TERM_WIDTH + 5);
    else
        fout << path;

    fout << "]\n  ";
#ifdef DEBUG
    fout << "[" << current.get_uid() << "] ";
#endif
    fout << "[Rating: " << current.rating.mean;
    fout << "(" << current.rating.dev << ")";
    fout << setiosflags(std::ios::showpos);
    if (current.relation)
        fout << current.relation << "r";
    if (current.acoustic)
        fout << current.acoustic << "a";
    fout << resetiosflags(std::ios::showpos);

    fout << "] [Last: " << strtime(current.last_played) <<
        (current.last_played == local_max ?  "!" : "");

    fout << "] ";

    fout << (!current.identified ? "[Unknown] " : "");
    fout << (!current.get_playcounter() ? "[New] " : "");

    fout.flush();
}

void Imms::set_lastinfo(LastInfo &last)
{
    last.set_on = time(0);
    last.uid = current.get_uid();
    last.sid = current.get_sid();
    last.avalid = current.get_acoustic(&last.mm, last.beats);
}

void Imms::end_song(bool at_the_end, bool jumped, bool bad)
{
    time_t played = at_the_end ? 10 : 5;
    int flags = 0;

    if (bad)
    {
        played = 50;
        flags |= Flags::bad;
    }

    if (jumped)
        flags |= Flags::jumped_from;
    if (last_jumped)
        flags |= Flags::jumped_to;

    if ((!at_the_end && !last_skipped)
            || (at_the_end && last_skipped))
        flags |= Flags::first;

    if (xidle_enabled)
    {
        flags |= Flags::idleness;
        if (XIdle::is_active())
            flags |= Flags::active;
    }

    last_skipped = !at_the_end;

#ifdef DEBUG
    cerr << " *** " << path_get_filename(current.get_path()) << endl;
#endif

    if (at_the_end && (!xidle_enabled || flags & Flags::active))
        set_lastinfo(last);

    if (at_the_end && (flags & Flags::first || flags & Flags::jumped_to))
        set_lastinfo(handpicked);

    last_jumped = jumped;

    Song::Rating r;

    AutoTransaction at;

    ImmsDb::add_recent(current.get_uid(), played, flags);
    r = current.update_rating();
    current.set_last(time(0));
    current.increment_playcounter();

    at.commit();

    fout << (jumped ? "[Jumped] " : "");
    fout << (!jumped && last_skipped ? "[Skipped] " : "");
    fout << "[After: " << r.print() << "]";
    fout << endl;

}

void Imms::evaluate_transition(SongData &data, LastInfo &last, float weight)
{
    // Reset lasts if we had them for too long
    if (last.sid != -1 && last.set_on + LAST_EXPIRE < time(0))
        last.sid = -1;

    if (last.sid == -1)
        return;

    float rel = cap(ImmsDb::correlate(
                data.get_sid(), last.sid) / MAX_CORRELATION);
    data.relation += ROUND(rel * weight * CORRELATION_IMPACT);

    if (!last.avalid)
        return;

    MixtureModel mm;
    float beats[BEATSSIZE];
    if (!data.get_acoustic(&mm, beats))
        return;

    float score = model.evaluate(last.mm, last.beats, mm, beats);
    data.acoustic += ROUND(score * weight * ACOUSTIC_IMPACT);
}

bool Imms::fetch_song_info(SongData &data)
{
    if (!InfoFetcher::fetch_song_info(data))
        return false;

    if (data.last_played > local_max)
        data.last_played = local_max;

    data.acoustic = data.relation = 0;

    evaluate_transition(data, handpicked, 0.75);
    evaluate_transition(data, last, (handpicked.sid == -1 ? 0.5 : 0.25));

    return true;
}
