#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>     // for (s)random

#include <iostream>
#include <iomanip>

#include "imms.h"
#include "strmanip.h"

using std::string;
using std::endl;
using std::cerr;
using std::setprecision;

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
#define     HOUR                    (60*60)
#define     DAY                     (24*HOUR)

#define     TERM_WIDTH              80

//////////////////////////////////////////////

// Random
int imms_random(int max)
{
    int rand_num;
    static struct random_data rand_data;
    static char rand_state[256];
    static bool initialized = false;
    if (!initialized)
    {
        initstate_r(time(0), rand_state, sizeof(rand_state), &rand_data);
        initialized = true;
    }
    random_r(&rand_data, &rand_num);
    double cof = rand_num / (RAND_MAX + 1.0);
    return (int)(max * cof);
}

// Imms
Imms::Imms() : last_handpicked(-1)
{
    last_skipped = last_jumped = false;
    local_max = MAX_TIME;

    string homedir = getenv("HOME");
    fout.open(homedir.append("/.imms/imms.log").c_str(),
            ofstream::out | ofstream::app);

    time_t t = time(0);
    fout << endl << endl << ctime(&t) << setprecision(3);
}

void Imms::setup(const char* _email, bool use_xidle)
{
    email = _email;
    xidle_enabled = use_xidle;
}

void Imms::pump()
{
    XIdle::query();
}

void Imms::playlist_changed(int playlist_size)
{
    local_max = playlist_size * 8 * 60;

    if (local_max > MAX_TIME)
        local_max = MAX_TIME;

    history.clear();
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
    SongPicker::reset();
    SpectrumAnalyzer::reset();

    revalidate_winner(path);

    history.push_back(position);

    immsdb.set_id(winner.id);
    immsdb.set_last(time(0));

    print_song_info();
}

void Imms::print_song_info()
{
    fout << string(TERM_WIDTH - 15, '-') << endl << "[";

    if (winner.path.length() > TERM_WIDTH - 2)
        fout << "..." << winner.path.substr(winner.path.length()
                - TERM_WIDTH + 5);
    else
        fout << winner.path;

    fout << "]\n  [Rating: " << winner.rating;
    if (winner.relation)
        fout << setiosflags(std::ios::showpos) << winner.relation
            << resetiosflags(std::ios::showpos) << "x";

    fout << "] [Last: " << strtime(winner.last_played) <<
        (winner.last_played == local_max ?  "!" : "") << "] ";

    fout << (!winner.identified ? "[Unknown] " : "");
    fout << (winner.unrated ? "[New] " : "");

    fout.flush();
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

        last_skipped = jumped = false;
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

        last_skipped = true;
    }

    if (bad)
        mod = 0;

    immsdb.set_id(winner.id);

    SpectrumAnalyzer::finalize();

    if (mod > CONS_NON_SKIP_RATE + INTERACTIVE_BONUS)
        last_handpicked = winner.id.second;

    fout << (jumped ? "[Jumped] " : "");
    fout << (!jumped && last_skipped ? "[Skipped] " : "");
    fout << "[Delta " << setiosflags(std::ios::showpos) << mod <<
        resetiosflags (std::ios::showpos) << "] ";
    fout << endl;

    last_jumped = jumped;

    if (abs(mod) > CONS_NON_SKIP_RATE)
        immsdb.add_recent(mod);

    int new_rating = winner.rating + mod;
    if (new_rating > MAX_RATING)
        new_rating = MAX_RATING;
    else if (new_rating < MIN_RATING)
        new_rating = MIN_RATING;

    immsdb.set_rating(new_rating);
}

bool Imms::fetch_song_info(SongData &data)
{
    if (!InfoFetcher::fetch_song_info(data))
        return false;

    data.relation = 0;
    if (last_handpicked != -1)
        data.relation = immsdb.correlate(last_handpicked);

    if (data.relation > 15)
        data.relation = 15;

    if (data.last_played > local_max)
        data.last_played = local_max;

    return true;
}
