#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

#include <iostream>

#include "song.h"
#include "flags.h"
#include "md5digest.h"
#include "immsutil.h"
#include "sqlite++.h"
#include "strmanip.h"
#include "songinfo.h"
#include "normal.h"
#include "appname.h"

#define DELTA_SCALE     1.5
#define DECAY_LIMIT     60
#define MIN_TRIALS      30

using std::cerr;
using std::endl;

int Rating::sample()
{
    int rating = ROUND(normal(mean, dev));
    return std::min(100, std::max(0, rating));
}

int evaluate_artist(const string &artist, const string &album,
        const string &title, int count)
{
    int score = 0;

    score += album != "" ? 10 : -4;
    score += title != "" ? 5 : -10;

    score += count;

    int length = artist.length();
    int upper = 0, ascii = 0, alpha = 0, blanks = 0, uscore = 0;
    for (int i = 0; i < length; ++i) 
    {
        int c = artist[i];
        upper += isupper(c);
        ascii += isascii(c);
        alpha += isalpha(c);
        blanks += isblank(c);
        uscore += (c == '_' || c == '-');
    }

    if (uscore > 1 && blanks == 0)
        score -= 6;
    if (upper == length)
        score -= 4;
    if (upper > 0 && upper < length / 3)
        score += 10;
    if (length - ascii < 3)
        score += 10;

    if (isdigit(artist[0]))
        score -= 5;

    if (artist.find(" - ") != string::npos)
        score -= 8;
    if (title.find(" - ") != string::npos)
        score -= 10;

    return score;
}

Song::Song(const string &path_, int _uid, int _sid) : path(path_)
{
    reset();

    uid = _uid;
    sid = _sid;

    if (isok() || path == "")
        return;

    struct stat statbuf;
    if (stat(path.c_str(), &statbuf))
        return;

    try {
        identify(statbuf.st_mtime);

        AutoTransaction a(AppName != IMMSD_APP);
        Q("UPDATE Library SET lastseen = ? WHERE uid = ?")
            << time(0) << uid << execute;
        a.commit();
    }
    WARNIFFAILED();
}

void Song::get_tag_info(string &artist, string &album, string &title) const
{
    artist = album = title = "";

    Q q("SELECT artist, album, title FROM Tags WHERE uid = ?;");
    q << uid;

    if (!q.next())
        return;

    q >> artist >> album >> title;
}

bool Song::isanalyzed()
{
    try {
        Q q("SELECT * FROM A.Acoustic WHERE mfcc NOTNULL "
               "AND bpm NOTNULL AND uid = ?;");
        q << uid;
        if (q.next())
            return true;
    }
    WARNIFFAILED();
    return false;
}

void Song::set_acoustic(const void *mfccdat, size_t mfccsize,
        const void *bpmdat, size_t bpmsize)
{
    try {
        Q q("INSERT OR REPLACE INTO A.Acoustic "
                "('uid', 'mfcc', 'bpm') "
                "VALUES (?, ?, ?);");
        q << uid;
        q.bind(mfccdat, mfccsize);
        q.bind(bpmdat, bpmsize);
        q.execute();
    }
    WARNIFFAILED();
}

bool Song::get_acoustic(void *mfccdat, size_t mfccsize,
        void *bpmdat, size_t bpmsize)
{
    if (uid < 0)
        return false;

    try
    {
        Q q("SELECT mfcc, bpm FROM A.Acoustic WHERE uid = ?;");
        q << uid;

        if (q.next())
        {
            if (mfccdat)
                q.load(mfccdat, mfccsize);  
            if (bpmdat)
                q.load(bpmdat, bpmsize);  
            return true;
        }
    }
    WARNIFFAILED();

    return false;
}

void Song::update_tag_info(const string &artist, const string &album,
        const string &title)
{
    // don't erase existing tags
    if (artist == "" && title == "")
    {
        Q q("SELECT count(1) FROM Tags WHERE uid = ?;");
        q << uid;
        if (q.next())
            return;
    }

    {
        Q q("INSERT OR REPLACE INTO Tags "
                "('uid', 'artist', 'album', 'title') "
                "VALUES (?, ?, ?, ?);");
        q << uid << artist << album << title;
        q.execute();
    }

    if (artist == "")
        return;
    
    int count = 0;
    {
        Q q("SELECT count(1) FROM Tags WHERE artist = ?;");
        q << artist;
        if (q.next())
            q >> count;
    }

    int trust = evaluate_artist(artist, album, title, count);
    int oldtrust = 0, aid = -1;

    {
        Q q("SELECT A.aid, A.artist, A.trust "
                "FROM Library L NATURAL INNER JOIN Info I "
                "INNER JOIN Artists A on I.aid = A.aid WHERE L.uid = ?;");
        q << uid;
        if (q.next())
            q >> aid >> oldtrust;
    }

    if (aid < 0 || oldtrust >= trust)
        return;

    try {
        Q q("UPDATE Artists SET readable = ?, trust = ? WHERE aid = ?;");
        q << artist << trust << aid;
        q.execute();
    } catch (SQLException &e) {}
}

void Song::identify(time_t modtime)
{
    {
        Q q("SELECT Library.uid, sid, modtime "
                "FROM Identify NATURAL JOIN 'Library' "
                "WHERE path = ?;");
        q << path;

        if (q.next())
        {
            time_t last_modtime;
            q >> uid >> sid >> last_modtime;

            if (modtime == last_modtime)
                return;
        }
    }

    string checksum = Md5Digest::digest_file(path);

    {
        AutoTransaction a(AppName != IMMSD_APP);
        _identify(modtime, checksum);
        a.commit();
    }

    SongInfo info;
    info.link(path);

    string artist = info.get_artist();
    string album = info.get_album();
    string title = info.get_title();

    {
        AutoTransaction a(AppName != IMMSD_APP);
        update_tag_info(artist, album, title);
        a.commit();
    }
}

void Song::_identify(time_t modtime, const string &checksum)
{
    // old path but modtime has changed - update checksum
    if (uid != -1)
    {
        Q q("UPDATE Identify SET modtime = ?, "
                "checksum = ? WHERE path = ?;");
        q << modtime << checksum << path;
        q.execute();
        return;
    }

    // moved or new file and path needs updating
    reset();

    Q q("SELECT uid, path FROM Identify WHERE checksum = ?;");
    q << checksum;

    bool duplicate;
    if (duplicate = q.next())
    {
        // Check if any of the old paths no longer exist 
        // (aka file was moved) so that we can reuse their uid
        do
        {
            string oldpath;
            q >> uid >> oldpath;

            if (access(oldpath.c_str(), F_OK))
            {
                q.reset();

                sid = -1;

                Q("UPDATE Identify SET path = ?, "
                        "modtime = ? WHERE path = ?;")
                    << path << modtime << oldpath << execute;

                Q("UPDATE Library SET sid = -1 WHERE uid = ?;")
                    << uid << execute;
#ifdef DEBUG
                cerr << "identify: moved: uid = " << uid << endl;
#endif
                return;
            }
        } while (q.next());
    }
    else
    {
        // figure out what the next uid should be
        Q q("SELECT max(uid) FROM Library;");
        if (q.next())
            q >> uid;
        ++uid;
    }

#ifdef DEBUG
    cerr << "identify: new: uid = " << uid << endl;
#endif

    // new file - insert into the database
    Q("INSERT INTO Identify "
            "('path', 'uid', 'modtime', 'checksum') "
            "VALUES (?, ?, ?, ?);")
        << path << uid << modtime << checksum << execute;

    if (!duplicate)
        Q("INSERT INTO Library "
                "('uid', 'sid', 'playcounter', 'lastseen', 'firstseen') "
                "VALUES (?, ?, ?, ?, ?);")
            << uid << -1 << 0 << time(0) << time(0) << execute;
}

void Song::set_last(time_t last)
{
    if (uid < 0)
        return;

    try {
        AutoTransaction a;

        if (sid < 0)
            register_new_sid();

        Q q("INSERT OR REPLACE INTO Last ('sid', 'last') VALUES (?, ?);");
        q << sid << last;
        q.execute();

        a.commit();
    }
    WARNIFFAILED();
}

int Song::get_playcounter()
{
    if (uid < 0)
        return -1;

    if (playcounter != -1)
        return playcounter;

    try
    {
        Q q("SELECT playcounter FROM Library WHERE uid = ?;");
        q << uid;

        if (q.next())
            q >> playcounter;
    }
    WARNIFFAILED();

    return playcounter;
}

void Song::increment_playcounter()
{
    if (uid < 0)
        return;

    try
    { 
        Q("UPDATE Library SET playcounter = playcounter + 1 WHERE uid = ?;")
            << uid << execute;
    }
    WARNIFFAILED();
}

void Song::set_rating(const Rating &rating)
{
    if (uid < 0)
        return;

    try
    {
        Q q("INSERT OR REPLACE INTO Ratings "
               "('uid', 'rating', 'dev') VALUES (?, ?, ?);");
        q << uid << rating.mean << rating.dev;
        q.execute();
    }
    WARNIFFAILED();
}

void Song::set_info(const StringPair &info)
{
    if (uid < 0)
        return;

    artist = info.first;
    title = info.second;

    try {
        AutoTransaction a;

        int aid = -1;
        {
            Q q("SELECT aid FROM Artists WHERE artist = ?;");
            q << artist;

            if (q.next())
                q >> aid;
            else
            {
                Q("INSERT INTO Artists (artist) VALUES (?);")
                    << artist << execute;
                aid = SQLDatabaseConnection::last_rowid();
            }
        }

        sid = -1;
        Q q("SELECT sid FROM Info WHERE aid = ? AND title = ?;");
        q << aid << title;
        if (q.next())
        {
            q >> sid;
            q.execute();

            {
                Q q("UPDATE Library SET sid = ? WHERE uid = ?;");
                q << sid << uid;
                q.execute();
            }
        }
        else
        {
            register_new_sid();

            Q q("INSERT INTO Info ('sid', 'aid', 'title') VALUES (?, ?, ?);");
            q << sid << aid << title;
            q.execute();
        }

        a.commit();
    }
    WARNIFFAILED();

#ifdef DEBUG
    cerr << " >> cached as: " << endl;
    cerr << " >>     artist  = '" << artist << "'" << endl;
    cerr << " >>     title   = '" << title << "'" << endl;
#endif
}

time_t Song::get_last()
{
    if (sid < 0)
        return 0;

    time_t result = 0;

    try
    {
        Q q("SELECT last FROM Last WHERE sid = ?;");
        q << sid;
        if (q.next())
            q >> result;
    }
    WARNIFFAILED();
    return result;
}

Rating Song::get_raw_rating()
{
    Rating r;

    if (uid < 0)
        return r;

    try
    {
        Q q("SELECT rating, dev FROM Ratings WHERE uid = ?;");

        q << uid;
        if (q.next())
        {
            q >> r.mean >> r.dev;
            return r;
        }

        AutoTransaction a;

        infer_rating();
        update_rating();

        q << uid;
        if (q.next())
        {
            q >> r.mean >> r.dev;
            q.execute();
        }

        a.commit();
    }
    WARNIFFAILED();

    return r;
}

void Song::infer_rating()
{
    if (sid < 0)
        return;

    int mean = -1, trials;

    try {
        Q q("SELECT avg(rating), sum(playcounter) "
                "FROM Library L NATURAL JOIN Ratings "
                "WHERE L.sid = ?;");
        q << sid;

        // if we have another instance of the same song,
        // take it's rating as the default
        if (q.next())
            q >> mean >> trials;

        if (mean <= 0)
        {
            // otherwise base it on the average of the artist as a whole
            int aid = -1;
            {
                Q q("SELECT aid FROM Info WHERE sid = ?;");
                q << sid;
                if (q.next())
                    q >> aid;
            }
            if (aid > 0)
            {
                Q q("SELECT avg(rating), sum(playcounter)/sum(1) "
                        "FROM Library L NATURAL JOIN Info I "
                        "INNER JOIN Ratings R on L.uid = R.uid "
                        "WHERE aid = ?;");
                q << aid;
                if (q.next())
                {
                    q >> mean >> trials;
                    mean = std::max(33, std::min(66, mean));
                }
            }
        }

        if (mean > 0)
        {
            Q q("INSERT INTO Bias ('uid', 'mean', 'trials') "
                    "VALUES (?, ?, ?);");
            q << uid << mean << trials;
            q.execute();
        }
    } WARNIFFAILED();
}

int Song::get_rating(Rating *r_)
{
    Rating r = r_ ? *r_ : get_raw_rating();
    return r.sample();
}

StringPair Song::get_info()
{
    if (sid < 0)
        return StringPair("", "");

    if (artist != "" && title != "")
        return StringPair(artist, title);

    artist = title = "";

    try
    {
        Q q("SELECT title, artist "
                "FROM Info NATURAL INNER JOIN Artists WHERE sid = ?;");
        q << sid;

        if (q.next())
            q >> title >> artist;
    }
    WARNIFFAILED();

#if defined(DEBUG) && 0
    if (title != "" && artist != "")
    {
        cerr << " >> from cache: " << endl;
        cerr << " >>     artist  = '" << artist << "'" << endl;
        cerr << " >>     title   = '" << title << "'" << endl;
    }
#endif
    return StringPair(artist, title);
}    

void Song::register_new_sid()
{
    Q q("SELECT max(sid) FROM Library;");
    if (q.next())
    {
        q >> sid;
        q.execute();
    }
    ++sid;

    Q("UPDATE Library SET sid = ? WHERE uid = ?;") << sid << uid << execute;

#ifdef DEBUG
    cerr << __func__ << ": registered sid = " << sid << " for uid = "
        << uid << endl;
#endif
}

static float decay(float delta, float sum)
{
    if (sum > DECAY_LIMIT)
        return 0;

    return delta * log(DECAY_LIMIT + 1 - sum) / log(DECAY_LIMIT);
} 

Rating Song::update_rating()
{
    Rating r;

    if (uid < 0)
        return r;

    try
    {
        float biasmean = 0.5, biastrials = 0;
        {
            Q q("SELECT sum(mean * trials) / sum(trials), sum(trials) "
                    "FROM Bias WHERE uid = ? GROUP BY uid;");
            q << uid;

            if (q.next())
            {
                q >> biasmean >> biastrials;
                biasmean /= 100.0;
            }
        }

        Q q("SELECT played, flags FROM Journal WHERE uid = ? "
                "ORDER BY time DESC;");
        q << uid;

        double total = 0, ones = 0, zeros = 0;

        while (q.next())
        {
            int flags;
            time_t played;
            q >> played >> flags;
            double delta = Flags::deltify(played, flags) * DELTA_SCALE;
            if (delta > 0)
                ones += decay(delta, total);
            else
                zeros += decay(-delta, total);
            total += fabs(delta);
        }

        if (!ones && !zeros)
            zeros = ones = 1;

        double biasmass = 0;
        double sum = ones + zeros;

        if (total < MIN_TRIALS)
        {
            biasmass = (MIN_TRIALS - total);
            sum += biastrials * biasmass / MIN_TRIALS;

        }

        ones += biasmass * biasmean;
        zeros += biasmass * (1 - biasmean);

        double p = ones / (ones + zeros);
        r.mean = ROUND(100.0 * p);
        r.dev = ROUND(100.0 * sqrt(p * (1 - p) / sum));

#if 0
        DEBUGVAL(biasmass);
        DEBUGVAL(biasmean);
        DEBUGVAL(total);
        DEBUGVAL(sum);
        DEBUGVAL(ones);
        DEBUGVAL(zeros);
        DEBUGVAL(r.mean);
        DEBUGVAL(r.dev);
#endif

        set_rating(r);
    }
    WARNIFFAILED();

    return r;
}
