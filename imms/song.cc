#include "song.h"
#include "md5digest.h"
#include "sqlite++.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

using std::cerr;
using std::endl;

Song Song::identify(const string &path)
{
    Song song;

    struct stat statbuf;
    if (stat(path.c_str(), &statbuf))
        return Song();

    time_t modtime = statbuf.st_mtime;

    try {
        AutoTransaction a;

        {
            Q q("SELECT uid, sid, modtime FROM 'Library' WHERE path = ?;");
            q << path;

            if (q.next())
            {
                time_t last_modtime;
                q >> song.uid >> song.sid >> last_modtime;

                if (modtime == last_modtime)
                    return song;
            }
        }

        string checksum = Md5Digest::digest_file(path);

        // old path but modtime has changed - update checksum
        if (song.uid != -1)
        {
            Q q("UPDATE 'Library' SET modtime = ?, "
                    "checksum = ? WHERE path = ?';");
            q << modtime << checksum << path;
            q.execute();
            a.commit();
            return song;
        }

        // moved or new file and path needs updating
        song.reset();

        Q q("SELECT uid, sid, path FROM 'Library' WHERE checksum = ?;");
        q << checksum;

        if (q.next())
        {
            // Check if any of the old paths no longer exist 
            // (aka file was moved) so that we can reuse their uid
            do
            {
                string oldpath;
                q >> song.uid >> song.sid >> oldpath;

                if (access(oldpath.c_str(), F_OK))
                {
                    q.reset();

                    {
                        Q q("UPDATE 'Library' SET sid = -1, "
                                "path = ?, modtime = ? WHERE path = ?;");

                        q << path << modtime << oldpath;

                        q.execute();
                    }
#ifdef DEBUG
                    cerr << "identify: moved: uid = " << song.uid << endl;
#endif
                    a.commit();
                    return song;
                }
            } while (q.next());
        }
        else
        {
            // figure out what the next uid should be
            Q q("SELECT max(uid) FROM Library;");
            if (q.next())
                q >> song.uid;
            ++song.uid;
        }

        {
            // new file - insert into the database
            Q q("INSERT INTO 'Library' "
                    "('uid', 'sid', 'path', 'modtime', 'checksum') "
                    "VALUES (?, -1, ?, ?, ?);");

            q << song.uid << path << modtime << checksum;

            q.execute();
        }

#ifdef DEBUG
        cerr << "identify: new: uid = " << song.uid << endl;
#endif

        a.commit();
        return song;
    }
    WARNIFFAILED();

    return Song();
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

void Song::set_rating(int rating)
{
    if (uid < 0)
        return;

    try
    {
        Q q("INSERT OR REPLACE INTO 'Rating' ('uid', 'rating') VALUES (?, ?);");
        q << uid << rating;
        q.execute();
    }
    WARNIFFAILED();
}

void Song::set_artist(const string &_artist) { artist = _artist; }

void Song::set_title(const string &_title)
{
    if (uid < 0)
        return;

    title = _title;

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

    cerr << __func__ << " registered sid = " << sid << " for uid = "
        << uid << endl;
}
