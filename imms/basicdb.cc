#include <iostream>
#include <unistd.h>
#include <math.h>

#include "strmanip.h"
#include "immsdb.h"
#include "utils.h"

using std::endl;
using std::cerr; 

BasicDb::BasicDb() : Song("")
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
    try {
        QueryCacheDisabler qdb;

        Q("PRAGMA cache_size = 10000").execute();
        Q("PRAGMA synchronous = OFF;").execute();
        Q("PRAGMA temp_store = MEMORY;").execute();
    }
    WARNIFFAILED();
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

        Q("CREATE TABLE 'Acoustic' ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'spectrum' TEXT, 'bpm' TEXT);").execute();

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

int BasicDb::avg_rating(const string &artist, const string &title)
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
                int avg;
                q >> avg;
                return avg;
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
                int avg;
                q >> avg;
                if (avg)
                    return std::min(std::max(avg, 90), 115);
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

bool BasicDb::check_title(const string &artist, string &title)
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
        if (from < 7)
        {
            Q("DROP TABLE Acoustic;").execute();
            // Create new tables
            sql_create_tables();
        }

        a.commit();
    }
    WARNIFFAILED();
}
