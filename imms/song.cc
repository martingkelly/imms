#include "song.h"
#include "md5digest.h"
#include "sqlite++.h"
#include "strmanip.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

using std::cerr;
using std::endl;

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

        Q("UPDATE 'Library' SET lastseen = ? WHERE uid = ?")
            << time(0) << uid << execute;
    }
    WARNIFFAILED();
}


void Song::identify(time_t modtime)
{
    AutoTransaction a;

    {
        Q q("SELECT Library.uid, sid, modtime "
                "FROM 'Identify' NATURAL JOIN 'Library' "
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

    // old path but modtime has changed - update checksum
    if (uid != -1)
    {
        Q q("UPDATE 'Identify' SET modtime = ?, "
                "checksum = ? WHERE path = ?;");
        q << modtime << checksum << path;
        q.execute();

        a.commit();
        return;
    }

    // moved or new file and path needs updating
    reset();

    Q q("SELECT uid, path FROM 'Identify' WHERE checksum = ?;");
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

                Q("UPDATE 'Identify' SET path = ?, "
                        "modtime = ? WHERE path = ?;")
                    << path << modtime << oldpath << execute;

                Q("UPDATE 'Library' SET sid = -1 WHERE uid = ?;")
                    << uid << execute;
#ifdef DEBUG
                cerr << "identify: moved: uid = " << uid << endl;
#endif
                a.commit();
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
    Q("INSERT INTO 'Identify' "
            "('path', 'uid', 'modtime', 'checksum') "
            "VALUES (?, ?, ?, ?);")
        << path << uid << modtime << checksum << execute;

    if (!duplicate)
        Q("INSERT INTO 'Library' "
                "('uid', 'sid', 'playcounter', 'lastseen', 'firstseen') "
                "VALUES (?, ?, ?, ?, ?);")
            << uid << -1 << 0 << time(0) << time(0) << execute;

    a.commit();
    return;
}

void Song::set_last(time_t last)
{
    if (uid < 0)
        return;

    try {
        AutoTransaction a;

        if (sid < 0)
            register_new_sid();

        Q q("INSERT OR REPLACE INTO 'Last' ('sid', 'last') VALUES (?, ?);");
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
        Q q("SELECT playcounter FROM 'Library' WHERE uid = ?;");
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
        Q("UPDATE 'Library' SET playcounter = playcounter + 1 WHERE uid = ?;")
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

        Q("INSERT OR REPLACE INTO 'Rating' "
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
        Q("UPDATE 'Rating' SET trend = ? WHERE uid = ?;")
            << trend << uid << execute;
    }
    WARNIFFAILED();
}

void Song::set_acoustic(const string &spectrum, const string &bpmgraph)
{
    try
    {
        Q q("INSERT OR REPLACE INTO 'Acoustic' "
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
        Q q("SELECT spectrum, bpm FROM 'Acoustic' WHERE uid = ?;");
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

        sid = -1;
        Q q("SELECT sid FROM 'Info' WHERE artist = ? AND title = ?;");
        q << artist << title;
        if (q.next())
        {
            q >> sid;
            q.execute();

            {
                Q q("UPDATE 'Library' SET sid = ? WHERE uid = ?;");
                q << sid << uid;
                q.execute();
            }
        }
        else
        {
            register_new_sid();

            Q q("INSERT INTO 'Info' "
                    "('sid', 'artist', 'title') VALUES (?, ?, ?);");
            q << sid << artist << title;
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
        Q q("SELECT last FROM 'Last' WHERE sid = ?;");
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
        Q q("SELECT rating FROM 'Rating' WHERE uid = ?;");
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
        Q q("SELECT trend FROM 'Rating' WHERE uid = ?;");
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
        Q q("SELECT title, artist FROM 'Info' WHERE sid = ?;");
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
