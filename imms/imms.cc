#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>     // for (s)random

#include <iostream>
#include <iomanip>

#include "imms.h"
#include "strmanip.h"

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

static int bounds[] =
    { -1, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255 };

static int scales[] = 
    { 11000, 4100, 3600, 3000, 2400, 2000, 1700, 2000, 1500, 1000, 750,
        650, 550, 450, 200, 80 };

// BeatKeeper
BeatKeeper::BeatKeeper()
{
    beats = max_bass = average_bass = 0;
}

inline int weighted_merge(int v1, int v2, double v2_weight)
{ return ROUND(pow(sqrt(v1) * (1 - v2_weight) + sqrt(v2) * v2_weight, 2)); }

void BeatKeeper::integrate_bass(int bass)
{
    if (bass > average_bass + (max_bass - average_bass) * 0.8)
    {
        if (!above_average)
            ++beats;
        above_average = true;
    }

    if (bass < average_bass)
        above_average = false;

    if (bass > max_bass)
        max_bass = bass;

    max_bass = weighted_merge(max_bass, bass, 0.01);
    average_bass = weighted_merge(average_bass, bass, 0.05);
}

int BeatKeeper::get_BPM(time_t seconds)
{
    int bpm = ROUND(beats * 60.0 / seconds);
    beats = max_bass = average_bass = 0;
    above_average = false;
    return bpm;
}

// Imms
Imms::Imms() : last_handpicked(-1)
{
    last_skipped = last_jumped = false;
    have_candidates = attempts = 0;
    local_max = MAX_TIME;
    last_spectrum = "";

    string homedir = getenv("HOME");
    fout.open(homedir.append("/.imms/imms.log").c_str(),
            ofstream::out | ofstream::app);

    time_t t = time(0);
    fout << endl << endl << ctime(&t) << setprecision(3);
}

void Imms::setup(const char* _email, bool _use_xidle)
{
    email = _email;
    use_xidle = _use_xidle;
}

void Imms::pump()
{
    xidle.query();
}

void Imms::start_spectrum_analysis()
{
    spectrum_start_time = time(0);
    have_spectrums = 0;
}

void Imms::stop_spectrum_analysis()
{
    time_t seconds = time(0) - spectrum_start_time;

    int bpmv0 = bpm0.get_BPM(seconds);
    int bpmv1 = bpm1.get_BPM(seconds);

    int min = bpmv0;
    if (bpmv1 < min)
        min = bpmv1;

    bpm = min;

    cerr << "BPM: " << min << " (" << bpmv0 << "/" << bpmv1 << ")" << endl;
}

void Imms::integrate_spectrum(uint16_t _long_spectrum[long_spectrum_size])
{
    static char delay = 0;
    if (++delay % 32 == 0)
    {
        for (int i = 0; i < long_spectrum_size; ++i)
            long_spectrum[i] = (long_spectrum[i] * have_spectrums 
                    + _long_spectrum[i]) / (have_spectrums + 1);

        ++have_spectrums;
    }

    bpm0.integrate_bass(1000 * _long_spectrum[0] / scales[0]);
    bpm1.integrate_bass(1000 * _long_spectrum[1] / scales[1]);
}

void Imms::finalize_spectrum()
{
    if (!have_spectrums)
        return;

    string short_spectrum = "";

    for (int i = 0; i < short_spectrum_size; ++i)
    {
        double average = 0;
        for (int j = bounds[i] + 1; j <= bounds[i + 1]; ++j)
            average += long_spectrum[j];

        average /= (bounds[i + 1] - bounds[i]);
        char c = 'a' + ROUND(('z' - 'a') * average / scales[i]); 
        if (c > 'z')
            c = 'z';
                
        short_spectrum += c;
    }

#ifdef DEBUG
    cerr << "spectrum [" << short_spectrum << "]" << endl;
#endif

    if (have_spectrums > 100)
        immsdb.set_spectrum(short_spectrum);

    int distance = 0;

    if (last_spectrum.length() == short_spectrum.length()
            && (int)last_spectrum.length() == short_spectrum_size)
        for (int i = 0; i < short_spectrum_size; ++i)
            distance += ROUND(pow(short_spectrum[i] - last_spectrum[i], 2));

    last_spectrum = short_spectrum;
#ifdef DEBUG
    cerr << "difference from last: " << distance << endl;
#endif
}

void Imms::playlist_changed(int _playlist_size)
{
    playlist_size = _playlist_size;
    local_max = playlist_size * 8 * 60;

    if (local_max > MAX_TIME)
        local_max = MAX_TIME;
#ifdef DEBUG
    cerr << " *** playlist changed!" << endl;
    cerr << "local_max is now " << strtime(local_max) << endl;
#endif
}

void Imms::start_song(const string &path)
{
    xidle.reset();
    candidates.clear();
    have_candidates = attempts = 0;

    revalidate_winner(path);

    immsdb.set_id(winner.id);
    immsdb.set_last(time(0));

    memset(long_spectrum, 0, sizeof(long_spectrum));

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

        if (!use_xidle)
            mod += NO_XIDLE_BONUS;
        else if (xidle.is_active())
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
            if (!use_xidle)
                mod -= NO_XIDLE_BONUS;
            else if (xidle.is_active())
                mod -= INTERACTIVE_BONUS;
        }

        last_skipped = true;
    }

    if (bad)
        mod = 0;

    immsdb.set_id(winner.id);

    finalize_spectrum();

    if (mod > CONS_NON_SKIP_RATE + INTERACTIVE_BONUS)
        last_handpicked = winner.id.second;

    fout << (jumped ? "[Jumped] " : "");
    fout << (!jumped && last_skipped ? "[Skipped] " : "");
    fout << "[Delta " << setiosflags(std::ios::showpos) << mod <<
        resetiosflags (std::ios::showpos) << "] ";
    fout << "[BPM: " << bpm << "] ";
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
