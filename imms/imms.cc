#include <time.h>
#include <ctype.h>
#include <math.h>

#include <iostream>
#include <iomanip>

#include "imms.h"
#include "strmanip.h"
#include "player.h"
#include "utils.h"

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
#define     INTERACTIVE_BONUS       2
#define     JUMPED_FROM             -1
#define     JUMPED_TO_SKIPPED       1

#define     MAX_TIME                20*DAY

#define     MAX_CORRELATION         12.0
#define     PRIMARY                 0.80
#define     CORRELATION_IMPACT      40
#define     LAST_EXPIRE             2*HOUR

#define     TERM_WIDTH              80

//////////////////////////////////////////////

// Imms
Imms::Imms() 
{
    last_skipped = last_jumped = false;
    local_max = MAX_TIME;

    handpicked.set_on = 0;
    last.sid = handpicked.sid = -1;

    string homedir = getenv("HOME");
    fout.open(homedir.append("/.imms/imms.log").c_str(),
            ofstream::out | ofstream::app);

    time_t t = time(0);
    fout << endl << endl << ctime(&t) << setprecision(3);
}

void Imms::setup(bool use_xidle)
{
    xidle_enabled = use_xidle;
}

void Imms::do_idle_events()
{
    SongPicker::identify_more();
    ImmsServer::do_events();
}

void Imms::do_events()
{
    SongPicker::do_events();
    ImmsServer::do_events();
    XIdle::query();
}

void Imms::playlist_changed()
{
    cerr << "playlist changed" << endl;
    local_max = Player::get_playlist_length() * 8 * 60;
    if (local_max > MAX_TIME)
        local_max = MAX_TIME;

    history.clear();
    ImmsDb::clear_recent();
    SongPicker::reset();

    playlist_ready = false;

    InfoFetcher::playlist_changed();
} 

void Imms::reset_selection()
{
    SongPicker::reset();
    Player::reset_selection();

    local_max = ImmsDb::get_effective_playlist_length() * 8 * 60;
    if (local_max > MAX_TIME)
        local_max = MAX_TIME;
}

int Imms::get_previous()
{
    if (history.size() < 2)
        return -1;
    history.pop_back();
    int result = history.back();
    history.pop_back();
    return result;
}

void Imms::start_song(int position, const string &path)
{
    XIdle::reset();

    revalidate_current(position, path);

    history.push_back(position);
    if (history.size() > 10)
        history.pop_front();

    *static_cast<Song*>(this) = *static_cast<Song*>(&current);
    ImmsDb::set_last(time(0));

    print_song_info();
}

void Imms::print_song_info()
{
    fout << string(TERM_WIDTH - 15, '-') << endl << "[";

    if (current.path.length() > TERM_WIDTH - 2)
        fout << "..." << current.path.substr(current.path.length()
                - TERM_WIDTH + 5);
    else
        fout << current.path;

    fout << "]\n  [Rating: " << current.rating;
    fout << setiosflags(std::ios::showpos);
    if (current.relation)
        fout << current.relation << "r";
    fout << resetiosflags(std::ios::showpos);

    fout << "] [Last: " << strtime(current.last_played) <<
        (current.last_played == local_max ?  "!" : "") << "] ";

    fout << (!current.identified ? "[Unknown] " : "");
    fout << (current.unrated ? "[New] " : "");

    fout.flush();
}

void Imms::set_lastinfo(LastInfo &last)
{
    last.set_on = time(0);
    last.sid = current.get_sid();
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

        if (!xidle_enabled)
            mod += NO_XIDLE_BONUS;
        else if (XIdle::is_active())
            mod += INTERACTIVE_BONUS;
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
            if (!xidle_enabled)
                mod -= NO_XIDLE_BONUS;
            else if (XIdle::is_active())
                mod -= INTERACTIVE_BONUS;
        }
    }

    last_skipped = !at_the_end;

    if (bad)
        mod = 0;

    *static_cast<Song*>(this) = *static_cast<Song*>(&current);

#ifdef DEBUG
    cerr << " *** " << path_get_filename(current.path) << endl;
#endif

    if (mod >= CONS_NON_SKIP_RATE)
        set_lastinfo(last);

    if (mod > CONS_NON_SKIP_RATE + INTERACTIVE_BONUS)
        set_lastinfo(handpicked);

    fout << (jumped ? "[Jumped] " : "");
    fout << (!jumped && last_skipped ? "[Skipped] " : "");
    fout << "[Delta " << setiosflags(std::ios::showpos) << mod <<
        resetiosflags (std::ios::showpos) << "] ";
    fout << endl;

    last_jumped = jumped;

    ImmsDb::add_recent(mod);

    int new_rating = current.rating + mod;
    if (new_rating > MAX_RATING)
        new_rating = MAX_RATING;
    else if (new_rating < MIN_RATING)
        new_rating = MIN_RATING;

    ImmsDb::set_last(time(0));
    ImmsDb::set_rating(new_rating);
}

float rescale(float score) { return score < 0 ? score * 2 : score; }

void Imms::evaluate_transition(SongData &data, LastInfo &last, float weight)
{
    // Reset lasts if we had them for too long
    if (last.sid != -1 && last.set_on + LAST_EXPIRE < time(0))
        last.sid = -1;

    if (last.sid == -1)
        return;

    float rel = ImmsDb::correlate(last.sid) / MAX_CORRELATION;
    rel = rel > 1 ? 1 : rel < -1 ? -1 : rel;
    data.relation += ROUND(rel * weight * CORRELATION_IMPACT);
}

bool Imms::fetch_song_info(SongData &data)
{
    if (!InfoFetcher::fetch_song_info(data))
        return false;

    if (data.last_played > local_max)
        data.last_played = local_max;

    data.relation = 0;

    evaluate_transition(data, handpicked, PRIMARY);
    evaluate_transition(data, last, 1 - PRIMARY);

    return true;
}
