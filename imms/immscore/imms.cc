#include <time.h>
#include <ctype.h>
#include <math.h>

#include <iostream>
#include <iomanip>

#include "imms.h"
#include "flags.h"
#include "strmanip.h"
#include "immsutil.h"

using std::string;
using std::endl;
using std::cerr;
using std::setprecision;
using std::ofstream;

//////////////////////////////////////////////
// Constants

#define     MAX_TIME                21*DAY

#define     MAX_CORRELATION         12.0
#define     SECONDARY               0.3
#define     CORRELATION_IMPACT      40

#define     LAST_EXPIRE             HOUR

#define     TERM_WIDTH              80

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

void Imms::get_metacandidates()
{
    AutoTransaction a;
    if (last.sid != -1)
        CorrelationDb::get_related(metacandidates, last.sid, 20);
    if (handpicked.sid != -1)
        CorrelationDb::get_related(metacandidates, handpicked.sid, 30);
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
    local_max = pl_length * 8 * 60;
    if (local_max > MAX_TIME)
        local_max = MAX_TIME;

    ImmsDb::clear_recent();
    SongPicker::playlist_changed(length);
} 

void Imms::playlist_ready()
{
    PlaylistDb::playlist_ready();
    SongPicker::playlist_ready();
}

void Imms::reset_selection()
{
    SongPicker::reset();
    server->reset_selection();

    local_max = ImmsDb::get_effective_playlist_length() * 8 * 60;
        local_max = MAX_TIME;
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
            set_lastinfo(handpicked);

        at.commit();
    } 
    WARNIFFAILED();

    // FIXME:
    if (0) {
        string epath = rex.replace(path, "'", "'\"'\"'", Regexx::global);
        system(string("analyzer '" + epath + "' &").c_str());
    }
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
    if (current.bpmrating)
        fout << current.bpmrating << "b";
    if (current.specrating)
        fout << current.specrating << "s";
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
    last.sid = current.get_sid();
    // FIXME:
    // last.acoustic = current.get_acoustic();
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

    Rating r;

    AutoTransaction at;

    ImmsDb::add_recent(current.get_uid(), played, flags);
    r = current.update_rating();
    current.set_last(time(0));
    current.increment_playcounter();

    at.commit();

    fout << (jumped ? "[Jumped] " : "");
    fout << (!jumped && last_skipped ? "[Skipped] " : "");
    fout << "[After: " << r.mean << "(" << r.dev << ")]";
    fout << endl;

}

void Imms::evaluate_transition(SongData &data, LastInfo &last, float weight)
{
    // Reset lasts if we had them for too long
    if (last.sid != -1 && last.set_on + LAST_EXPIRE < time(0))
        last.sid = -1;

    if (last.sid == -1)
        return;

    float rel = ImmsDb::correlate(data.get_sid(), last.sid) / MAX_CORRELATION;
    rel = std::max(std::min(rel, (float)1), (float)-1);
    data.relation += ROUND(rel * weight * CORRELATION_IMPACT);
}

bool Imms::fetch_song_info(SongData &data)
{
    if (!InfoFetcher::fetch_song_info(data))
        return false;

    if (data.last_played > local_max)
        data.last_played = local_max;

    data.specrating = data.bpmrating = data.relation = 0;

    evaluate_transition(data, handpicked, 1);
    evaluate_transition(data, last,
            SECONDARY * (handpicked.sid == -1 ? 2 : 1));

    return true;
}
