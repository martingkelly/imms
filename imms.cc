#include <iomanip>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>   // for mkdir
#include <sys/types.h>
#include <stdlib.h>     // for (s)random

#include <list>
#include <iostream>
#include <algorithm>

#include "imms.h"
#include "strmanip.h"
#include "md5digest.h"

using std::endl;
using std::cerr;
using std::setprecision;
using std::list;


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

#define     SAMPLE_SIZE             40
#define     MIN_SAMPLE_SIZE         15
#define     MAX_ATTEMPTS            (SAMPLE_SIZE*2)
#define     BASE_BIAS               10
#define     DISPERSION_FACTOR       4.0
#define     MAX_TIME                20*DAY
#define     MAX_RATING              150
#define     MIN_RATING              75
#define     CORRELATION_FACTOR      3

#define     TERM_WIDTH              80
#define     HOUR                    60*60
#define     DAY                     (24*HOUR)


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

// SongData
SongData::SongData(int _position, const string &_path)
    : position(_position), path(path_simplifyer(_path))
{
    rating = relation = 0;
    identified = unrated = false;
    last_played = 0;
}

// Imms
Imms::Imms() : last_handpicked(-1)
{
    winner_valid = last_skipped = last_jumped = false;
    have_candidates = attempts = 0;
    local_max = MAX_TIME;

    string dotimms = getenv("HOME");
    mkdir(dotimms.append("/.imms").c_str(), 0700);

    fout.open(dotimms.append("/imms.log").c_str(),
            ofstream::out | ofstream::app);

    time_t t = time(0);
    fout << endl << endl << ctime(&t) << setprecision(3);

    immsdb.reset(new ImmsDb());
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

void Imms::playlist_changed(int _playlist_size)
{
    playlist_size = _playlist_size;
    local_max = playlist_size * 8 * 60;
#ifdef DEBUG
    cerr << " *** playlist changed!" << endl;
    cerr << "local_max is now " << strtime(local_max) << endl;
#endif
}

bool Imms::add_candidate(int playlist_num, string path, bool urgent)
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

int Imms::select_next()
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
    cerr << string(TERM_WIDTH - 15, '-') << endl;
    cerr << " >> [" << min_composite << "-" << max_composite << "] ";
#endif

    int weight_index = imms_random(total);

    for (i = candidates.begin(); i != candidates.end(); ++i)
    { 
        weight_index -= i->composite_rating;
        if (weight_index < 0)
        {
            winner = *i;
            winner_valid = true;
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

void Imms::start_song(const string &path)
{
    xidle.reset();
    candidates.clear();
    have_candidates = attempts = 0;

    if (!winner_valid)
    {
        winner.path = path_simplifyer(path);
        fetch_song_info(winner);
    }

    immsdb->set_id(winner.id);
    immsdb->set_last(time(0));

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

    immsdb->set_id(winner.id);

    if (mod > CONS_NON_SKIP_RATE + INTERACTIVE_BONUS)
        last_handpicked = winner.id.second;

    fout << (jumped ? "[Jumped] " : "");
    fout << (!jumped && last_skipped ? "[Skipped] " : "");
    fout << "[Delta " << setiosflags(std::ios::showpos) << mod <<
        resetiosflags (std::ios::showpos) << "] ";
    fout << endl;

    last_jumped = jumped;
    winner_valid = false;

    if (abs(mod) > CONS_NON_SKIP_RATE)
        immsdb->add_recent(mod);

    int new_rating = winner.rating + mod;
    if (new_rating > MAX_RATING)
        new_rating = MAX_RATING;
    else if (new_rating < MIN_RATING)
        new_rating = MIN_RATING;

    immsdb->set_rating(new_rating);
}

bool Imms::fetch_song_info(SongData &data)
{
    const string &path = data.path;
    if (access(path.c_str(), R_OK))
        return false;

    struct stat statbuf;
    stat(path.c_str(), &statbuf);

    if (immsdb->identify(path, statbuf.st_mtime) < 0)
    {
        if (immsdb->identify(path, statbuf.st_mtime,
                Md5Digest::digest_file(path)) < 0)
            return false;
    }

    songinfo.link(path);
    data.rating = immsdb->get_rating();

    data.unrated = false;
    if (data.rating < 0)
    {
        data.unrated = true;
        data.rating = songinfo.get_rating(email);
        immsdb->set_rating(data.rating);
    }

    StringPair info = immsdb->get_info();
    artist = info.first;
    title = info.second;

    if (artist != "" && title != "")
        data.identified = true;
    else if ((data.identified = parse_song_info(path)))
        immsdb->set_title(title);

    data.relation = 0;
    if (last_handpicked != -1)
        data.relation = immsdb->correlate(last_handpicked);

    if (data.relation > 15)
        data.relation = 15;

#ifdef DEBUG
    cerr << "path:\t" << path << endl;
#if 0
    cerr << "path:\t" << path << endl;
    cerr << "artist:\t" << artist << (identified ? " *" : "") << endl;
    cerr << "title:\t" << title << (identified ? " *" : "") << endl;
#endif
#endif

    data.last_played = time(0) - immsdb->get_last();

    if (data.last_played > local_max)
        data.last_played = local_max;

    if (data.last_played > MAX_TIME)
        data.last_played = MAX_TIME;

    data.id = immsdb->get_id();

    return true;
}

bool Imms::parse_song_info(const string &path)
{

#define REMIXCLUES "rmx|mix|[^a-z]version|edit|original|remaster|" \
    "cut|instrumental|extended"
#define BADARTIST "artist|^va$|various|collection|^misc"

    //////////////////////////////////////////////
    // Initialize

    bool identified = false;
    bool parser_confident, artist_confirmed = false;

    string tag_artist = artist = string_normalize(songinfo.get_artist());

    StringPair fm = get_simplified_filename_mask(path);
    fm.second = string_normalize(fm.second);

    list<string> file_parts;
    parser_confident = imms_magic_parse_filename(file_parts, fm.first);

    // Can't normalize before we pass it to magic parse
    fm.first = string_normalize(fm.first);

    list<string> path_parts;
    imms_magic_parse_path(path_parts, path_get_dirname(path));

#ifdef DEBUG
#if 1
    cerr << "path parts: " << endl;
    for (list<string>::iterator i = path_parts.begin();
            i != path_parts.end(); ++i)
    {
        cerr << " >> " << *i << endl;
    }
    cerr << "name parts: " << endl;
    for (list<string>::iterator i = file_parts.begin();
            i != file_parts.end(); ++i)
    {
        cerr << " >> " << *i << endl;
    }
#endif
#endif

    //////////////////////////////////////////////
    // Try to identify the artist
    
    bool bad_tag_artist = tag_artist == "" || Regexx(tag_artist, BADARTIST);

    // See if we can recognize any of the path parts
    if (file_parts.size() > 1)
    {
        for (list<string>::iterator i = file_parts.begin();
                !artist_confirmed && i != file_parts.end(); ++i)
        {
            if (immsdb->check_artist(*i)
                    || (!bad_tag_artist && string_like(*i, tag_artist, 4)))
            {
                artist_confirmed = true;
                artist = *i;
            }
        }
    } 

    // See if any of the directories look familiar
    string outermost = "";
    for (list<string>::iterator i = path_parts.begin();
            !artist_confirmed && i != path_parts.end(); ++i)
    {
        if (Regexx(*i, BADARTIST))
            continue;
        if (artist_confirmed =
                (immsdb->check_artist(*i) || string_like(*i, artist, 4)))
            artist = *i;
        // And while we are at it find the uppermost related directory
        else if (outermost == "" && fm.first.find(*i) != string::npos)
            outermost = *i;
    }

    // The path part that is also the outermost directory might be the artist
    if (!artist_confirmed && outermost != "" && fm.second.find(outermost) < 2)
    {
        artist = outermost;
        artist_confirmed = true;
    }

    if (!artist_confirmed && parser_confident && file_parts.size() > 1)
    {
        artist = file_parts.front();
        artist_confirmed = true;
    }

    // Give the tag artist an opportunity to override the guessed artist
    if (!bad_tag_artist && !string_like(tag_artist, artist, 4))
    {
        if (immsdb->check_artist(tag_artist)
                || fm.first.find(tag_artist) != string::npos)
        {
            artist = tag_artist;
            artist_confirmed = true;
        }
    }

    // If we still don't know what to do - give up
    if (!artist_confirmed)
        return identified;

    // Erase the first occurrence of the artist
    list<string>::iterator i = find(file_parts.begin(), file_parts.end(), artist);
    if (i != file_parts.end())
        file_parts.erase(i);

    // By this point the artist had to have been confirmed
    immsdb->set_artist(artist);

    //////////////////////////////////////////////
    // Try (not very hard) to identify the album

    string album = album_filter(songinfo.get_album());
    string directory = album_filter(path_parts.back());
    if (album == "" || Regexx(directory, album))
        album = directory;

    //////////////////////////////////////////////
    // Try to identify the title

    string tag_title = string_tolower(songinfo.get_title());
    title = title_filter(tag_title);
    // If we know the title and were only missing the artist
    if (title != "" && (identified = immsdb->check_title(title)))
        return identified;

    if (identified = Regexx(title, album + ".*(" REMIXCLUES ")"))
        title = album;
    else if (identified = Regexx(fm.first, album + "$"))
        title = album;

    // Can recognize the title by matching filename parts to the database?
    for (list<string>::reverse_iterator i = file_parts.rbegin();
            !identified && i != file_parts.rend(); ++i)
    {
        if (immsdb->check_title(*i) || string_like(*i, title, 4))
        {
            identified = true;
            title = *i;
        }
        else if (identified = Regexx(*i, album + ".*(" REMIXCLUES ")"))
            title = album;
    }

    if (identified)
        return identified;

    // See if we can trust the tag title enough to just default to it
    if (title != "" && !Regexx(title, "(" REMIXCLUES "|^track$|^title$)")
            && tag_artist != "" && !Regexx(tag_artist, BADARTIST)
            && string_like(tag_title, title, 6))
    {
        return identified = true;
    }

    // Filter things that are probably not the title
    for (list<string>::iterator i = file_parts.begin();
            i != file_parts.end(); ++i)
    {
        if (Regexx(*i, "(" REMIXCLUES "|^track$|^title$)"))
            *i = "###";
    }

    file_parts.remove("###");

    // Default to the right most non-suspicious entry as the title
    if (parser_confident && file_parts.size())
    {
        identified = true;
        title = file_parts.back();
    }
    // nothing left. maybe a remix album??
    else if (!file_parts.size() && immsdb->check_title(album))
    {
        identified = true;
        title = album;
    }
    return identified;
}
