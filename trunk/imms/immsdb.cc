#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "strmanip.h"
#include "immsdb.h"

using std::endl;
using std::cerr; 

#define SCHEMA_VERSION 2

#define CORRELATION_TIME    (15*30)   // n * 30 ==> n minutes
#define MAX_CORR_STR        "15"
#define MAX_CORRELATION     15
#define SECOND_DEGREE       0.5

#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)

ImmsDb::ImmsDb()
    : SqlDb(string(getenv("HOME")).append("/.imms/imms.db"))
{
    sql_schema_upgrade();
    sql_create_tables();
}

ImmsDb::~ImmsDb()
{
    expire_recent("");
}

void ImmsDb::sql_create_tables()
{
    run_query(
            "CREATE TABLE 'Library' ("
                "'uid' INTEGER NOT NULL, "
                "'sid' INTEGER DEFAULT '-1', "
                "'path' VARCHAR(4096) UNIQUE NOT NULL, "
                "'modtime' TIMESTAMP NOT NULL, "
                "'checksum' VARCHAR(34) NOT NULL);");

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

    run_query(
            "CREATE TABLE 'Correlations' ("
                "'origin' INTEGER NOT NULL, "
                "'destination' INTEGER NOT NULL, "
                "'weight' INTEGER DEFAULT '0');");

    run_query(
            "CREATE TEMPORARY TABLE 'Recent' ("
                "'sid' INTEGER NOT NULL, "
                "'weight' INTEGER NOT NULL, "
                "'time' TIMESTAMP);");
}

void ImmsDb::add_recent(int weight)
{
    expire_recent("WHERE time < '" + itos(time(0) - CORRELATION_TIME) + "'");

    if (sid != -1)
        run_query("INSERT INTO 'Recent' "
                "VALUES ('" + itos(sid) + "', '"
                + itos(weight) + "', '"
                + itos(time(0)) + "')");
}

void ImmsDb::expire_recent(const string &where_clause)
{
    select_query(
            "SELECT sid, weight FROM 'Recent' " + where_clause + " LIMIT 5;",
            (SqlCallback)&ImmsDb::expire_recent_callback_1, 2);
}

int ImmsDb::expire_recent_callback_1(int argc, char **argv)
{
    assert(argc == 2);

    from = atoi(argv[0]);
    from_weight = atoi(argv[1]);

    run_query("DELETE FROM 'Recent' WHERE sid = '" + itos(from) + "';");

    select_query("SELECT sid, weight FROM 'Recent';",
            (SqlCallback)&ImmsDb::expire_recent_callback_2);

    return 0;
}

int ImmsDb::expire_recent_callback_2(int argc, char **argv)
{
    assert(argc == 2);

    to = atoi(argv[0]);
    to_weight = atoi(argv[1]);

    if (to == from)
        return 0;

    if (from_weight < 0 && to_weight < 0)
        return 0;

#ifdef DEBUG
    cerr << string(55, '-') << endl;
    cerr << "processing update between " << MIN(from, to) <<
        " and " << MAX(from, to) << endl;
#endif

    weight = sqrt(abs(from_weight*to_weight));

    if (from_weight < 0 || to_weight < 0)
        weight = -weight;

    // Update the primary link
    update_correlation(from, to, weight);

    if (fabs(weight) <= 3)
        return 0;

    // Update secondary links
    select_query(
            "SELECT * FROM 'Correlations' "
            "WHERE ("
                    "origin = '" + itos(to) + "' "
                    "OR origin = '" + itos(from) + "' "
                    "OR destination = '" + itos(to) + "' "
                    "OR destination = '" + itos(from) + "'"
                ") AND " + (weight > 0 ? "abs" : "") + " (weight) > 1 "
            "LIMIT 15;",
            (SqlCallback)&ImmsDb::update_secondaty_correlations, 3);
    return 0;
}

int ImmsDb::update_secondaty_correlations(int argc, char **argv)
{
    assert(argc == 3);

    int node1 = atoi(argv[0]), node2 = atoi(argv[1]);

    // Don't update the primary link again
    if ((node1 == to && node1 == from) || (node1 == from && node2 == to))
        return 0;

    node1 = (node1 == to ? from : (node1 == from) ? to : node1);
    node2 = (node2 == to ? from : (node2 == from) ? to : node2);

    float scale = atof(argv[2]) * SECOND_DEGREE / MAX_CORRELATION;

    update_correlation(node1, node2, weight * scale);

    return 0;
}

void ImmsDb::update_correlation(int from, int to, float weight)
{
#ifdef DEBUG
    cerr << " >> Updating link from " << MIN(from, to) << " to "
        << MAX(from, to) << " by " << weight << endl;
#endif

    string min = itos(MIN(from, to)), max = itos(MAX(from, to));

    // Make sure the link we are about to update exists
    run_query(
        "UPDATE 'Correlations' SET weight = "
             "max(min(weight + '" + itos(weight) + "', "
              MAX_CORR_STR "), -" MAX_CORR_STR ") "
        "WHERE origin = '" + min + "' AND destination = '" + max + "';");

    if (!changes())
        run_query("INSERT INTO 'Correlations' "
            " ('origin', 'destination', 'weight') "
                "VALUES ('" + min + "', '" + max + "', '"
                + itos(weight) + "');");
}

int ImmsDb::correlate(int from)
{
    if (sid == -1)
        return 0;
    
    select_query(
            "SELECT weight FROM 'Correlations' "
            "WHERE origin = '" + itos(MIN(from, sid)) + "' "
                "AND destination = '" + itos(MAX(from, sid)) + "';");

    return nrow && resultp[1] ? atoi(resultp[1]) : 0;
}

int ImmsDb::identify(const string &path, time_t modtime)
{
    title = artist = "";

    select_query(
            "SELECT uid, sid, modtime FROM 'Library' "
            "WHERE path = '" + escape_string(path) + "';");

    sid = uid = -1;

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

int ImmsDb::identify(const string &path, time_t modtime, const string &checksum)
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

bool ImmsDb::check_artist(string &artist)
{
    select_query(
            "SELECT artist FROM 'Info' "
            "WHERE artist LIKE '" + artist + "' LIMIT 1;");

    if (nrow && resultp[1])
    {
        artist = resultp[1];
        return true;
    }
    return false;
}

bool ImmsDb::check_title(string &title)
{
    select_query(
            "SELECT title FROM 'Info' "
            "WHERE artist = '" + artist + "' "
                "AND title LIKE '" + title + "' LIMIT 1;");

    if (nrow && resultp[1])
    {
        title = resultp[1];
        return true;
    }
    return false;
}

IntPair ImmsDb::get_id()
{
    return IntPair(uid, sid);
}

StringPair ImmsDb::get_info()
{
    if (sid == -1)
        return StringPair("", "");

    select_query(
            "SELECT title, artist FROM 'Info' "
            "WHERE sid = '" + itos(sid) + "';");

    artist = nrow ? resultp[3] : "";
    title = nrow ? resultp[2] : "";
#ifdef DEBUG
#if 0
    if (title != "" && artist != "")
    {
        cerr << " >> from cache: " << endl;
        cerr << " >>     artist  = '" << artist << "'" << endl;
        cerr << " >>     title   = '" << title << "'" << endl;
    }
#endif
#endif
    return StringPair(artist, title);
}

time_t ImmsDb::get_last()
{
    if (sid == -1)
        return 0;

    select_query(
            "SELECT last FROM 'Last' "
            "WHERE sid = '" + itos(sid) + "';");

    return nrow && resultp[1] ? atol(resultp[1]) : 0;
}

int ImmsDb::get_rating()
{
    if (uid == -1)
        return -1;

    select_query(
            "SELECT rating FROM 'Rating' "
            "WHERE uid = '" + itos(uid) + "';");

    return nrow ? atoi(resultp[1]) : -1;
}

void ImmsDb::set_artist(const string &_artist)
{
    artist = _artist;
}

void ImmsDb::set_title(const string &_title)
{
    if (uid == -1)
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

void ImmsDb::register_new_sid(int new_sid)
{
    if (new_sid == -1)
    {
        select_query("SELECT max(sid) FROM Library;");
        new_sid = resultp[1] ? atoi(resultp[1]) + 1 : 1;
    }
    else
    {
        run_query(
                "UPDATE 'Correlations' SET origin = '" + itos(new_sid) + "' "
                "WHERE origin = '" + itos(sid) + "';");

        run_query(
                "UPDATE 'Correlations' SET destination = '" + itos(new_sid) + 
                "' WHERE destination = '" + itos(sid) + "';");
    }

    sid = new_sid;

    run_query(
            "UPDATE 'Library' SET sid = '" + itos(sid) + "' "
            "WHERE uid = '" + itos(uid) + "';");
}

void ImmsDb::set_last(time_t last)
{
    if (uid == -1)
        return;

    if (sid == -1)
        register_new_sid();

    run_query(
            "INSERT OR REPLACE INTO 'Last' ('sid', 'last') "
            "VALUES ('" + itos(sid) + "', '" +  itos(last) + "');");
}


void ImmsDb::set_id(const IntPair &p)
{
    uid = p.first;
    sid = p.second;
}

void ImmsDb::set_rating(int rating)
{
    if (uid == -1)
        return;

    run_query(
            "INSERT OR REPLACE INTO 'Rating' ('uid', 'rating') "
            "VALUES ('" + itos(uid) + "', '" +  itos(rating) + "');");
}

void ImmsDb::sql_schema_upgrade()
{
    select_query("SELECT version FROM 'Schema' WHERE description ='latest';");

    if (nrow && resultp[1] && atoi(resultp[1]) > SCHEMA_VERSION)
    {
        cerr << "IMMS: Newer database schema detected." << endl;
        cerr << "IMMS: Please update IMMS!" << endl;
        close_database();
        return;
    }

    if (nrow && resultp[1] && atoi(resultp[1]) == SCHEMA_VERSION)
        return;

    cerr << "IMMS: Outdated database schema detected." << endl;
    cerr << "IMMS: Attempting to update." << endl;

    run_query("DROP TABLE Info;");
    run_query("DROP TABLE Last;");
    run_query("DROP TABLE UnknownLast;");

    // Backup the existing tables
    run_query("CREATE TEMP TABLE Library_backup AS SELECT * FROM Library;");
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

    run_query(
            "INSERT OR REPLACE INTO 'Schema' ('description', 'version') "
            "VALUES ('latest', '" +  itos(SCHEMA_VERSION) + "');");
}
