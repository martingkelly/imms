#include <iostream>
#include <unistd.h>
#include <math.h>

#include "strmanip.h"
#include "immsdb.h"

using std::endl;
using std::cerr; 

BasicDb::BasicDb()
    : SqlDb(string(getenv("HOME")).append("/.imms/imms.db"))
{
    sql_set_pragma();
}

BasicDb::~BasicDb()
{
}

void BasicDb::sql_set_pragma()
{
    run_query("PRAGMA cache_size = 10000");
    run_query("PRAGMA synchronous = OFF;");
    run_query("PRAGMA temp_store = MEMORY;");
}

void BasicDb::sql_create_tables()
{
    run_query(
            "CREATE TABLE 'Library' ("
                "'uid' INTEGER NOT NULL, "
                "'sid' INTEGER DEFAULT '-1', "
                "'path' VARCHAR(4096) UNIQUE NOT NULL, "
                "'modtime' TIMESTAMP NOT NULL, "
                "'checksum' VARCHAR(34) NOT NULL);");

    run_query(
            "CREATE TABLE 'Acoustic' ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'bpm' INTEGER DEFAULT '0', "
                "'spectrum' VARCHAR(16) DEFAULT NULL);");

    run_query(
            "CREATE TABLE 'Rating' ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'rating' INTEGER NOT NULL);");

    run_query(
            "CREATE TABLE 'Info' ("
                "'sid' INTEGER UNIQUE NOT NULL," 
                "'artist' VARCHAR(255) NOT NULL, "
                "'title' VARCHAR(255) NOT NULL);");

    run_query(
            "CREATE TABLE 'Last' ("
                "'sid' INTEGER UNIQUE NOT NULL, " 
                "'last' TIMESTAMP);");
}


int BasicDb::identify(const string &path, time_t modtime)
{
    title = artist = "";
    sid = uid = -1;

    select_query(
            "SELECT uid, sid, modtime FROM 'Library' "
            "WHERE path = '" + escape_string(path) + "';");

    if (nrow)
    {
        uid = atoi(resultp[ncol]);
        sid = atoi(resultp[ncol + 1]);
        time_t last_modtime = atol(resultp[ncol + 2]);

        if (modtime == last_modtime)
            return uid;
    }

    return -1;
}

int BasicDb::identify(const string &path, time_t modtime,
        const string &checksum)
{
    // old path but modtime has changed - update checksum
    if (nrow)
    {
        run_query(
                "UPDATE 'Library' SET "
                    "modtime = '" + itos(modtime) + "', "
                    "checksum = '" + checksum + "' "
                "WHERE path = '" + escape_string(path) + "';");
        return uid;
    }

    // moved or new file and path needs updating

    sid = uid = -1;

    select_query(
            "SELECT uid, sid, path FROM 'Library' "
            "WHERE checksum = '" + checksum + "';");

    if (nrow)
    {
        // Check if any of the old paths no longer exist 
        // (aka file was moved) so that we can reuse their uid
        
        for (int col = ncol; nrow--; col += ncol)
        {
            // We do not update the sid here because since
            // the path changed it might now parse differently
            uid = atoi(resultp[col]);

            if (access(resultp[col + 2], F_OK))
            {
                run_query(
                        "UPDATE 'Library' SET "
                            "sid = '-1', "
                            "path = '" + escape_string(path) + "', "
                            "modtime = '" + itos(modtime) + "' "
                        "WHERE path = '" + escape_string(resultp[col + 2])
                        + "';");

#ifdef DEBUG
                cerr << "identify: moved: uid = " << uid << endl;
#endif

                return uid;
            }
        }
    }
    else
    {
        // figure out what the next uid should be
        select_query("SELECT max(uid) FROM Library;");
        uid = resultp[1] ? atoi(resultp[1]) + 1 : 1;
    }

    // new file - insert into the database
    run_query(
            "INSERT INTO 'Library' ('uid', 'path', 'modtime', 'checksum') "
            "VALUES ('" +
                itos(uid) + "', '" +
                escape_string(path) + "', '" +
                itos(modtime) + "', '" +
                checksum + "'"
            ");");

#ifdef DEBUG
    cerr << "identify: new: uid = " << uid << endl;
#endif

    return uid;
}

int BasicDb::avg_rating()
{
    if (title != "")
    {
        select_query(
                "SELECT avg(rating) FROM Library "
                    "NATURAL INNER JOIN Info "
                    "INNER JOIN Rating ON Library.uid = Rating.uid "
                    "WHERE Info.artist = '" + artist + "' "
                    "AND Info.title = '" + title + "';");

        if (nrow && resultp[1] && (int)atof(resultp[1]))
            return (int)atof(resultp[1]);
    }

    if (artist != "")
    {
        select_query(
                "SELECT avg(rating) FROM Library "
                    "NATURAL INNER JOIN Info"
                    "INNER JOIN Rating ON Rating.uid = Library.uid"
                    "WHERE Info.artist = '" + artist + "';");

        if (nrow && resultp[1] && (int)atof(resultp[1]))
            return (int)atof(resultp[1]);
    }

    return -1;
}

bool BasicDb::check_artist(string &artist)
{
    select_query(
            "SELECT artist FROM 'Info' "
            "WHERE similar(artist, '" + artist + "') LIMIT 1;");

    if (nrow && resultp[1])
    {
        artist = resultp[1];
        return true;
    }
    return false;
}

bool BasicDb::check_title(string &title)
{
    select_query(
            "SELECT title FROM 'Info' "
            "WHERE artist = '" + artist + "' "
                "AND similar(title, '" + title + "') LIMIT 1;");

    if (nrow && resultp[1])
    {
        title = resultp[1];
        return true;
    }
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

    select_query(
            "SELECT title, artist FROM 'Info' "
            "WHERE sid = '" + itos(sid) + "';");

    artist = nrow ? resultp[3] : "";
    title = nrow ? resultp[2] : "";
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

string BasicDb::get_spectrum()
{
    if (uid < 0)
        return "";

    select_query(
            "SELECT spectrum, bpm FROM 'Acoustic' "
            "WHERE uid = '" + itos(uid) + "';");

    bpm = nrow && resultp[3] ? atoi(resultp[3]) : 0;

    return nrow && resultp[2] ? resultp[2] : "";
}

int BasicDb::get_bpm()
{
    if (uid < 0)
        return 0;

    return bpm;
}

time_t BasicDb::get_last()
{
    if (sid < 0)
        return 0;

    select_query(
            "SELECT last FROM 'Last' "
            "WHERE sid = '" + itos(sid) + "';");

    return nrow && resultp[1] ? atol(resultp[1]) : 0;
}

int BasicDb::get_rating()
{
    if (uid < 0)
        return -1;

    select_query(
            "SELECT rating FROM 'Rating' "
            "WHERE uid = '" + itos(uid) + "';");

    return nrow ? atoi(resultp[1]) : -1;
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

    select_query(
            "SELECT sid FROM 'Info' "
            "WHERE artist = '" + artist + "' AND title = '" + title + "';");

    register_new_sid(nrow && resultp[1] ? atoi(resultp[1]) : sid);

    run_query(
            "INSERT INTO 'Info' ('sid', 'artist', 'title') "
            "VALUES ('" + itos(sid) + "', '" + artist + "', '" + title + "');");

#ifdef DEBUG
    cerr << " >> cached as: " << endl;
    cerr << " >>     artist  = '" << artist << "'" << endl;
    cerr << " >>     title   = '" << title << "'" << endl;
#endif
}

void BasicDb::register_new_sid(int new_sid)
{
    if (new_sid < 0)
    {
        select_query("SELECT max(sid) FROM Library;");
        new_sid = resultp[1] ? atoi(resultp[1]) + 1 : 1;
    }
    else
    {
        run_query(
                "UPDATE 'Correlations' SET "
                    "origin = '" + itos(new_sid) + "', "
                    "key = '" + itos(new_sid) + "'||'|'||destination "
                "WHERE origin = '" + itos(sid) + "';");

        run_query(
                "UPDATE 'Correlations' SET "
                    "destination = '" + itos(new_sid) + "', "
                    "key = origin||'|'||'" + itos(new_sid) + "' "
                "WHERE destination = '" + itos(sid) + "';");
    }

    sid = new_sid;

    run_query(
            "UPDATE 'Library' SET sid = '" + itos(sid) + "' "
            "WHERE uid = '" + itos(uid) + "';");
}

void BasicDb::set_last(time_t last)
{
    if (uid < 0)
        return;

    if (sid < 0)
        register_new_sid();

    run_query(
            "INSERT OR REPLACE INTO 'Last' ('sid', 'last') "
            "VALUES ('" + itos(sid) + "', '" +  itos(last) + "');");
}


void BasicDb::set_id(const IntPair &p)
{
    uid = p.first;
    sid = p.second;
}

void BasicDb::set_spectrum(const string &spectrum)
{
    if (uid < 0)
        return;

    run_query("INSERT INTO 'Acoustic' ('uid') VALUES ('" + itos(uid) + "');");

    run_query(
            "UPDATE 'Acoustic' SET spectrum = '" + spectrum +  "' "
            "WHERE uid = '" + itos(uid) + "';");
}

void BasicDb::set_bpm(int bpm)
{
    if (uid < 0)
        return;

    run_query(
            "UPDATE 'Acoustic' SET bpm = '" + itos(bpm) +  "' "
            "WHERE uid = '" + itos(uid) + "';");
}

void BasicDb::set_rating(int rating)
{
    if (uid < 0)
        return;

    run_query(
            "INSERT OR REPLACE INTO 'Rating' ('uid', 'rating') "
            "VALUES ('" + itos(uid) + "', '" +  itos(rating) + "');");
}

void BasicDb::sql_schema_upgrade(int from)
{
    if (from < 2)
    {
        run_query("DROP TABLE Info;");
        run_query("DROP TABLE Last;");
        run_query("DROP TABLE UnknownLast;");

        // Backup the existing tables
        run_query("CREATE TEMP TABLE Library_backup "
                   "AS SELECT * FROM Library;");
        run_query("DROP TABLE Library;");

        run_query("CREATE TEMP TABLE Rating_backup AS SELECT * FROM Rating;");
        run_query("DROP TABLE Rating;");

        // Create new tables
        sql_create_tables();

        // Copy the data into new tables, and drop the backups
        run_query(
                "INSERT INTO Library (uid, path, modtime, checksum) "
                "SELECT * FROM Library_backup;");
        run_query("DROP TABLE Library_backup;");

        run_query("INSERT INTO Rating SELECT * FROM Rating_backup;");
        run_query("DROP TABLE Rating_backup;");

        // Mark the new version
        run_query(
                "CREATE TABLE 'Schema' ("
                "'description' VARCHAR(10) UNIQUE NOT NULL, "
                "'version' INTEGER NOT NULL);");
    }
    if (from < 4)
    {
        // Backup the existing tables
        run_query("CREATE TEMP TABLE Library_backup "
                   "AS SELECT * FROM Library;");
        run_query("DROP TABLE Library;");

        // Create new tables
        sql_create_tables();

        // Copy the data into new tables, and drop the backups
        run_query(
                "INSERT INTO Library (uid, sid, path, modtime, checksum) "
                "SELECT uid, sid, path, modtime, checksum "
                "FROM Library_backup;");
        run_query("DROP TABLE Library_backup;");
    }
}
