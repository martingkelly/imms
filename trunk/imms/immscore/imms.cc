#include <time.h>
#include <ctype.h>
#include <math.h>

#include <iostream>
#include <iomanip>

#include "imms.h"
#include "strmanip.h"
#include "immsutil.h"

using std::string;
using std::endl;
using std::cerr;
using std::setprecision;
using std::ofstream;

//////////////////////////////////////////////
// Constants

#define     FIRST_SKIP_RATE         -6
#define     CONS_SKIP_RATE          -4
#define     FIRST_NON_SKIP_RATE     5
#define     CONS_NON_SKIP_RATE      1
#define     JUMPED_TO_FINNISHED     7
#define     NO_XIDLE_BONUS          1
#define     JUMPED_FROM             -1
#define     JUMPED_TO_SKIPPED       1
#define     INTERACTIVE_BONUS       2

#define     MAX_TIME                21*DAY

#define     MAX_CORRELATION         12.0
#define     SECONDARY               0.3
#define     CORRELATION_IMPACT      40

#define     BPM_IMPACT              25
#define     SPECTRUM_IMPACT         10
#define     SPECTRUM_RADIUS         3.75
#define     BPM_RADIUS              2.5

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
    PlaylistDb::playlist_clear();
    SongPicker::playlist_changed(length);
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

        // FIXME:
        if (0) {
            string epath = rex.replace(path, "'", "'\"'\"'", Regexx::global);
            system(string("analyzer '" + epath + "' &").c_str());
        }
    } 
    WARNIFFAILED();
}

void Imms::print_song_info()
{
    fout << string(TERM_WIDTH - 15, '-') << endl << "[";

    const string &path = current.get_path();
    if (path.length() > TERM_WIDTH - 2)
        fout << "..." << path.substr(path.length() - TERM_WIDTH + 5);
    else
        fout << path;

    fout << "]\n  [Rating: " << current.rating;
    fout << setiosflags(std::ios::showpos);
    if (current.relation)
        fout << current.relation << "r";
    if (current.newness)
        fout << current.newness << "n";
    if (current.bpmrating)
        fout << current.bpmrating << "b";
    if (current.specrating)
        fout << current.specrating << "s";
    fout << resetiosflags(std::ios::showpos);

    fout << "] [Last: " << strtime(current.last_played) <<
        (current.last_played == local_max ?  "!" : "");

    if (current.trend_scale != 1)
        fout << "*" << current.trend_scale;
       
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
    int mod;

    if (at_the_end)
    {
        if (last_jumped)
            mod = JUMPED_TO_FINNISHED;
        else if (last_skipped)
            mod = FIRST_NON_SKIP_RATE;
        else
            mod = CONS_NON_SKIP_RATE;

        if (XIdle::is_active())
            mod += INTERACTIVE_BONUS;
        else if (!xidle_enabled)
            mod += NO_XIDLE_BONUS;
    }
    else
    {
        if (last_jumped && !jumped)
            mod = JUMPED_TO_SKIPPED;
        else if (jumped)
            mod = JUMPED_FROM;
        else if (last_skipped)
            mod = CONS_SKIP_RATE;
        else
            mod = FIRST_SKIP_RATE;

        if (mod < JUMPED_FROM)
        {
            if (XIdle::is_active())
                mod -= INTERACTIVE_BONUS;
            else if (!xidle_enabled)
                mod -= NO_XIDLE_BONUS;
        }
    }

    last_skipped = !at_the_end;

    if (bad)
        mod = 0;

#ifdef DEBUG
    cerr << " *** " << path_get_filename(current.get_path()) << endl;
#endif

    if (mod >= CONS_NON_SKIP_RATE)
        set_lastinfo(last);

    if (mod > CONS_NON_SKIP_RATE + INTERACTIVE_BONUS)
        set_lastinfo(handpicked);

    fout << (jumped ? "[Jumped] " : "");
    fout << (!jumped && last_skipped ? "[Skipped] " : "");
    fout << "[Delta " << setiosflags(std::ios::showpos) << mod <<
        resetiosflags(std::ios::showpos) << "] ";
    fout << endl;

    last_jumped = jumped;

    int new_rating = std::min(
            std::max(current.rating + mod, MIN_RATING), MAX_RATING);

    int trend = current.get_trend();

    if (abs(mod) > CONS_NON_SKIP_RATE + INTERACTIVE_BONUS)
    {
        if ((trend >= 0 && mod > 0) || (trend <= 0 && mod < 0))
        {
            trend += mod;
            if (abs(trend) > MAX_TREND)
                trend = trend > 0 ? MAX_TREND : - MAX_TREND;
        }
        else 
            trend = mod;
    }
    else
    {
        if (abs(trend) < 3)
            trend = 0;
        else
            trend += (trend > 0 ? -3 : 3);
    }

    AutoTransaction at;

    ImmsDb::add_recent(current.get_uid(), mod);
    current.set_trend(trend);
    current.set_last(time(0));
    current.set_rating(new_rating);
    current.increment_playcounter();

    at.commit();
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
