#include <time.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>     // for (s)random

#include <iostream>
#include <iomanip>

#include "imms.h"
#include "strmanip.h"
#include "player.h"

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
#define     HOUR                    (60*60)
#define     DAY                     (24*HOUR)

#define     MAX_CORRELATION         12.0
#define     SPECTRUM_IMPACT         10
#define     BPM_IMPACT              15
#define     PRIMARY                 0.80
#define     CORRELATION_IMPACT      40
#define     LAST_EXPIRE             2*HOUR

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
Imms::Imms() 
{
    last_skipped = last_jumped = false;
    local_max = MAX_TIME;

    handpicked.set_on = 0;
    last.sid = handpicked.sid = -1;
    handpicked.bpm = 0;

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

void Imms::reload_playlist()
{
#ifdef DEBUG
    cerr << "reloading playlist... ";
#endif
    immsdb.clear_playlist();

    for (int i = 0; i < Player::get_playlist_length(); ++i)
        immsdb.insert_playlist_item(i, Player::get_playlist_item(i));
#ifdef DEBUG
    cerr << "done" << endl;
#endif
}

void Imms::playlist_changed()
{
    local_max = Player::get_playlist_length() * 8 * 60;

    if (local_max > MAX_TIME)
        local_max = MAX_TIME;

    history.clear();
    immsdb.clear_recent();
    SongPicker::reset();

    reload_playlist();
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
    SpectrumAnalyzer::reset();

    revalidate_current(path);

    history.push_back(position);
    if (history.size() > 10)
        history.pop_front();

    immsdb.set_id(current.id);
    immsdb.set_last(time(0));

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
    if (current.color_rating)
        fout << current.color_rating << "s";
    if (current.bpm_rating)
        fout << current.bpm_rating << "b";
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
    last.sid = current.id.second;
    last.bpm = SpectrumAnalyzer::get_last_bpm();
    last.spectrum = SpectrumAnalyzer::get_last_spectrum();
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

    immsdb.set_id(current.id);

#ifdef DEBUG
    cerr << " *** " << path_get_filename(current.path) << endl;
#endif

    SpectrumAnalyzer::finalize();

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

    if (abs(mod) > CONS_NON_SKIP_RATE)
        immsdb.add_recent(mod);

    int new_rating = current.rating + mod;
    if (new_rating > MAX_RATING)
        new_rating = MAX_RATING;
    else if (new_rating < MIN_RATING)
        new_rating = MIN_RATING;

    immsdb.set_last(time(0));
    immsdb.set_rating(new_rating);
}

float rescale(float score) { return score < 0 ? score * 2 : score; }

void Imms::evaluate_transition(SongData &data, LastInfo &last, float weight)
{
    // Reset lasts if we had them for too long
    if (last.sid != -1 && last.set_on + LAST_EXPIRE < time(0))
        last.sid = -1;

    if (last.sid == -1)
        return;

    float rel = immsdb.correlate(last.sid) / MAX_CORRELATION;
    rel = rel > 1 ? 1 : rel < -1 ? -1 : rel;
    data.relation += ROUND(rel * weight * CORRELATION_IMPACT);

    if (data.spectrum != "" && last.spectrum != "")
        data.color_rating += ROUND(rescale(color_transition(
                last.spectrum, data.spectrum)) * weight * SPECTRUM_IMPACT);

    if (data.bpm_value && last.bpm)
        data.bpm_rating += ROUND(rescale(bpm_transition(
                    last.bpm, data.bpm_value)) * weight * BPM_IMPACT);
}

int Imms::fetch_song_info(SongData &data)
{
    int result = InfoFetcher::fetch_song_info(data);

    if (data.last_played > local_max)
        data.last_played = local_max;

    data.bpm_rating = data.color_rating = data.relation = 0;

    evaluate_transition(data, handpicked, PRIMARY);
    evaluate_transition(data, last, 1 - PRIMARY);

#if defined(DEBUG) && 1
    cerr << "[" << std::setw(60) << path_get_filename(data.path) << "] ";
    if (data.color_rating)
        cerr << "[ color = " << data.color_rating << " ] ";
    if (data.bpm_rating)
        cerr << "[ bpm = " << data.bpm_rating << " ] ";
    cerr << endl;
#endif

    return result;
}
