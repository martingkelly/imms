#include <iostream>
#include <unistd.h>
#include <math.h>

#include "strmanip.h"
#include "immsdb.h"
#include "utils.h"

using std::endl;
using std::cerr; 

BasicDb::BasicDb()
    : SqlDb(string(getenv("HOME")).append("/.imms/imms2.db"))
{
    sql_set_pragma();
}

BasicDb::~BasicDb()
{
    try {
        Q q("DELETE FROM 'Journal' WHERE time < ?;");
        q << time(0) - 30 * DAY ;
        q.execute();
    }
    WARNIFFAILED();
}

void BasicDb::sql_set_pragma()
{
    Q("PRAGMA cache_size = 10000").execute();
    Q("PRAGMA synchronous = OFF;").execute();
    Q("PRAGMA temp_store = MEMORY;").execute();
}

void BasicDb::sql_create_tables()
{
    QueryCacheDisabler qcd;
    RuntimeErrorBlocker reb;
    try
    {
        Q("CREATE TABLE 'Library' ("
                "'uid' INTEGER NOT NULL, "
                "'sid' INTEGER DEFAULT -1, "
                "'path' VARCHAR(4096) UNIQUE NOT NULL, "
                "'modtime' TIMESTAMP NOT NULL, "
                "'checksum' TEXT NOT NULL);").execute();

        Q("CREATE TABLE 'Rating' ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'rating' INTEGER NOT NULL);").execute();

        Q("CREATE TABLE 'Info' ("
                "'sid' INTEGER UNIQUE NOT NULL," 
                "'artist' TEXT NOT NULL, "
                "'title' TEXT NOT NULL);").execute();

        Q("CREATE TABLE 'Last' ("
                "'sid' INTEGER UNIQUE NOT NULL, " 
                "'last' TIMESTAMP);").execute();

        Q("CREATE TABLE 'Journal' ("
                "'uid' INTEGER NOT NULL, " 
                "'delta' INTEGER NOT NULL, " 
                "'time' TIMESTAMP NOT NULL);").execute();
    }
    WARNIFFAILED();
}

int BasicDb::avg_rating()
{
    try
    {
        if (title != "")
        {
            Q q("SELECT avg(rating) FROM Library "
                    "NATURAL INNER JOIN Info "
                    "INNER JOIN Rating ON Library.uid = Rating.uid "
                    "WHERE Info.artist = ? AND Info.title = ?;");

            q << artist << title;

            if (q.next())
            {
                float avg;
                q >> avg;
                return (int)avg;
            }
        }

        if (artist != "")
        {
            Q q("SELECT avg(rating) FROM Library "
                    "NATURAL INNER JOIN Info "
                    "INNER JOIN Rating ON Rating.uid = Library.uid "
                    "WHERE Info.artist = ?;");
            q << artist;

            if (q.next())
            {
                float avg;
                q >> avg;
                if (avg)
                {
                    int rating = (int)avg;
                    rating = std::min(rating, 115);
                    rating = std::max(rating, 90);
                    return rating;
                }
            }
        }
    }
    WARNIFFAILED();
    return -1;
}

bool BasicDb::check_artist(string &artist)
{
    try
    {
        Q q("SELECT artist FROM 'Info' WHERE similar(artist, ?);");
        q << artist;

        if (q.next())
        {
            q >> artist;
            return true;
        }
    }
    WARNIFFAILED();
    return false;
}

bool BasicDb::check_title(string &title)
{
    try
    {
        Q q("SELECT title FROM 'Info' WHERE artist = ? AND similar(title, ?)");
        q << artist << title;

        if (q.next())
        {
            q >> title;
            return true;
        }
    }
    WARNIFFAILED();
    return false;
}

IntPair BasicDb::get_id()
{
    return IntPair(uid, sid);
}

StringPair BasicDb::get_info()
{
    if (sid < 0)
        return StringPair("", "");

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

time_t BasicDb::get_last()
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

int BasicDb::get_rating()
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

void BasicDb::set_artist(const string &_artist)
{
    artist = _artist;
}

void BasicDb::set_title(const string &_title)
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

void BasicDb::set_last(time_t last)
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

void BasicDb::register_new_sid()
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

void BasicDb::set_id(const IntPair &p)
{
    title = artist = "";
    uid = p.first;
    sid = p.second;
}

void BasicDb::set_rating(int rating)
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

void BasicDb::sql_schema_upgrade(int from)
{
    QueryCacheDisabler qcd;
    RuntimeErrorBlocker reb;
    try 
    {
        AutoTransaction a;
        if (from < 4)
        {
            // Backup the existing tables
            Q("CREATE TEMP TABLE Library_backup "
                    "AS SELECT * FROM Library;").execute();
            Q("DROP TABLE Library;").execute();

            // Create new tables
            sql_create_tables();

            // Copy the data into new tables, and drop the backups
            Q("INSERT INTO Library (uid, sid, path, modtime, checksum) "
                    "SELECT uid, sid, path, modtime, checksum "
                    "FROM Library_backup;").execute();
            Q("DROP TABLE Library_backup;").execute();
        }

        a.commit();
    }
    WARNIFFAILED();
}
