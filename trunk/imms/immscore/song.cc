#include "song.h"
#include "md5digest.h"
#include "sqlite++.h"
#include "strmanip.h"
#include "songinfo.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#include <iostream>

using std::cerr;
using std::endl;

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

        Q("UPDATE Library SET lastseen = ? WHERE uid = ?")
            << time(0) << uid << execute;
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

void Song::update_tag_info()
{
    SongInfo info;
    info.link(path);

    string artist = info.get_artist();
    string album = info.get_album();
    string title = info.get_title();

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
    AutoTransaction a(true);

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

    _identify(modtime);

    update_tag_info();
    a.commit();
}

void Song::_identify(time_t modtime)
{
    string checksum = Md5Digest::digest_file(path);

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

void Song::set_rating(int rating)
{
    if (uid < 0)
        return;

    try
    {
        int trend = get_trend();

        Q("INSERT OR REPLACE INTO Rating "
               "('uid', 'rating', 'trend') VALUES (?, ?, ?);")
            << uid << rating << trend << execute;
    }
    WARNIFFAILED();
}

void Song::set_trend(int trend)
{
    if (uid < 0)
        return;

    try
    {
        Q("UPDATE Rating SET trend = ? WHERE uid = ?;")
            << trend << uid << execute;
    }
    WARNIFFAILED();
}

void Song::set_acoustic(const string &spectrum, const string &bpmgraph)
{
    try
    {
        Q q("INSERT OR REPLACE INTO A.Acoustic "
                "('uid', 'spectrum', 'bpm') VALUES (?, ?, ?);");
        q << uid << spectrum << bpmgraph;
        q.execute();
    }
    WARNIFFAILED();
}

StringPair Song::get_acoustic()
{
    StringPair res;
    try
    {
        Q q("SELECT spectrum, bpm FROM A.Acoustic WHERE uid = ?;");
        q << uid;
        if (q.next())
            q >> res.first >> res.second;
    }
    WARNIFFAILED();

    return res;
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

int Song::get_rating()
{
    if (uid < 0)
        return -1;

    int rating = -1;

    try
    {
        Q q("SELECT rating FROM Rating WHERE uid = ?;");
        q << uid;

        if (q.next())
            q >> rating;
    }
    WARNIFFAILED();
    return rating;
}

int Song::get_trend()
{
    if (uid < 0)
        return 0;

    int trend = 0;

    try
    {
        Q q("SELECT trend FROM Rating WHERE uid = ?;");
        q << uid;

        if (q.next())
            q >> trend;
    }
    WARNIFFAILED();
    return trend;
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
    {
        Q q("SELECT max(sid) FROM Library;");
        if (q.next())
            q >> sid;
        ++sid;
    }

    {
        Q q("UPDATE 'Library' SET sid = ? WHERE uid = ?;");
        q << sid << uid;
        q.execute();
    }

#ifdef DEBUG
    cerr << __func__ << ": registered sid = " << sid << " for uid = "
        << uid << endl;
#endif
}
