#include <iostream>
#include <unistd.h>
#include <math.h>

#include "flags.h"
#include "strmanip.h"
#include "immsdb.h"
#include "immsutil.h"

using std::endl;
using std::cerr; 
using Flags::undeltify; 

BasicDb::BasicDb()
{
    sql_set_pragma();
}

BasicDb::~BasicDb()
{
}

void BasicDb::sql_set_pragma()
{
    try {
        QueryCacheDisabler qdb;

        Q("PRAGMA cache_size = 5000").execute();
        Q("PRAGMA temp_store = MEMORY;").execute();
    }
    WARNIFFAILED();
}

void BasicDb::sql_create_v8_tables()
{
    QueryCacheDisabler qcd;
    RuntimeErrorBlocker reb;
    try
    {
        Q("CREATE TABLE Identify ("
                "'path' VARCHAR(4096) UNIQUE NOT NULL, "
                "'uid' INTEGER NOT NULL, "
                "'modtime' TIMESTAMP NOT NULL, "
                "'checksum' TEXT NOT NULL);").execute();
                
        Q("CREATE TABLE Library ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'sid' INTEGER DEFAULT -1, "
                "'playcounter' INTEGER DEFAULT 0, "
                "'lastseen' TIMESTAMP DEFAULT 0, "
                "'firstseen' TIMESTAMP DEFAULT 0);").execute();

        Q("CREATE TABLE Rating ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'rating' INTEGER NOT NULL, "
                "'trend' INTEGER DEFAULT 0);").execute();

        Q("CREATE TABLE Acoustic ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'spectrum' TEXT, 'bpm' TEXT);").execute();

        Q("CREATE TABLE Info ("
                "'sid' INTEGER UNIQUE NOT NULL," 
                "'artist' TEXT NOT NULL, "
                "'title' TEXT NOT NULL);").execute();

        Q("CREATE TABLE Last ("
                "'sid' INTEGER UNIQUE NOT NULL, " 
                "'last' TIMESTAMP);").execute();

        Q("CREATE TABLE Journal ("
                "'uid' INTEGER NOT NULL, " 
                "'delta' INTEGER NOT NULL, " 
                "'time' TIMESTAMP NOT NULL);").execute();
    }
    WARNIFFAILED();
}

void BasicDb::sql_create_tables()
{
    QueryCacheDisabler qcd;
    RuntimeErrorBlocker reb;
    try
    {
        Q("CREATE TABLE Identify ("
                "'path' VARCHAR(4096) UNIQUE NOT NULL, "
                "'uid' INTEGER NOT NULL, "
                "'modtime' TIMESTAMP NOT NULL, "
                "'checksum' TEXT NOT NULL);").execute();
                
        Q("CREATE TABLE Library ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'sid' INTEGER DEFAULT -1, "
                "'playcounter' INTEGER DEFAULT 0, "
                "'lastseen' TIMESTAMP DEFAULT 0, "
                "'firstseen' TIMESTAMP DEFAULT 0);").execute();

        Q("CREATE TABLE Ratings ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'rating' INTEGER NOT NULL, "
                "'dev' INTEGER DEFAULT 0);").execute();

        Q("CREATE TABLE A.Acoustic ("
                "'uid' INTEGER UNIQUE NOT NULL, "
                "'mfcc' BLOB DEFAULT NULL, "
                "'bpm' BLOB DEFAULT NULL);").execute();

        Q("CREATE TABLE A.Distances ("
                "'x' INTEGER NOT NULL, 'y' INTEGER NOT NULL, "
                "'dist' INTEGER NOT NULL);").execute();

        Q("CREATE UNIQUE INDEX A.Distances_x_y_i "
                "ON Distances (x, y);").execute();

        Q("CREATE TABLE Info ("
                "'sid' INTEGER UNIQUE NOT NULL," 
                "'aid' INTEGER NOT NULL, "
                "'title' TEXT NOT NULL);").execute();

        Q("CREATE TABLE Tags ("
                "'uid' INTEGER UNIQUE NOT NULL, " 
                "'title' TEXT NOT NULL, "
                "'album' TEXT NOT NULL, "
                "'artist' TEXT NOT NULL);").execute();

        Q("CREATE TABLE Artists ("
                "'aid' INTEGER PRIMARY KEY," 
                "'artist' TEXT UNIQUE NOT NULL, "
                "'readable' TEXT UNIQUE, "
                "'trust' INTEGER DEFAULT 0);").execute();

        Q("CREATE TABLE Last ("
                "'sid' INTEGER UNIQUE NOT NULL, " 
                "'last' TIMESTAMP);").execute();

        Q("CREATE TABLE Journal ("
                "'uid' INTEGER NOT NULL, " 
                "'played' TIME NOT NULL, " 
                "'flags' INTEGER NOT NULL, " 
                "'time' TIMESTAMP NOT NULL);").execute();

        Q("CREATE INDEX Jouranl_uid_i ON Journal (uid);").execute();

        Q("CREATE TABLE Bias ("
                "'uid' INTEGER NOT NULL, " 
                "'mean' INTEGER NOT NULL, " 
                "'trials' INTEGER NOT NULL);").execute();

        Q("CREATE INDEX Bias_uid_i ON Bias (uid);").execute();
    }
    WARNIFFAILED();
}

int BasicDb::avg_playcounter()
{
    static int playcounter = -1;
    if (playcounter != -1)
        return playcounter;

    try
    {
        Q q("SELECT avg(playcounter) from Library;");
        if (q.next() && q.not_null())
            q >> playcounter;
    }
    WARNIFFAILED();
    return playcounter;
}

bool BasicDb::check_artist(string &artist)
{
    try
    {
        Q q("SELECT artist FROM Artists WHERE similar(artist, ?);");
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
        Q q("SELECT title FROM Info NATURAL INNER JOIN Artists "
               "WHERE artist = ? AND similar(title, ?)");
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
    if (!from)
    {
        try
        {
            sql_create_tables();
        }
        WARNIFFAILED();
        return;
    }

    try 
    {
        QueryCacheDisabler qcd;
        AutoTransaction a;
        if (from < 7)
        {
            Q("DROP TABLE Acoustic;").execute();
        }
        if (from < 8)
        {
            // Backup the existing tables
            Q("CREATE TEMP TABLE Library_backup "
                    "AS SELECT * FROM Library;").execute();
            Q("DROP TABLE Library;").execute();

            // Backup the existing tables
            Q("CREATE TEMP TABLE Rating_backup "
                    "AS SELECT * FROM Rating;").execute();
            Q("DROP TABLE Rating;").execute();

            // Create new tables
            sql_create_v8_tables();

            Q("INSERT INTO Rating (uid, rating) "
                    "SELECT uid, rating FROM Rating_backup;").execute();
            Q("DROP TABLE Rating_backup;").execute();

            Q("INSERT INTO Identify (path, uid, modtime, checksum) "
                    "SELECT path, uid, modtime, checksum "
                    "FROM Library_backup;").execute();

            Q("INSERT OR REPLACE INTO Library "
                    "(uid, sid, playcounter, lastseen, firstseen) "
                    "SELECT uid, sid, 2, ?, ? FROM Library_backup;")
                 << time(0) << time(0) << execute;

            Q("DROP TABLE Library_backup;").execute();
        }
        if (from < 9)
        {
            // Backup the existing tables
            Q("CREATE TEMP TABLE Info_backup "
                    "AS SELECT * FROM Info;").execute();
            Q("DROP TABLE Info;").execute();

            // Create new tables
            sql_create_tables();

            Q("INSERT INTO Artists (artist) "
                    "SELECT DISTINCT(artist) FROM Info_backup;").execute();

            Q("INSERT INTO Info SELECT sid, aid, title "
                    "FROM Info_backup INNER JOIN Artists "
                        "ON Info_backup.artist = Artists.artist;").execute();
        }
        if (from < 10) 
        {
            sql_create_tables();

            Q("INSERT INTO A.Acoustic SELECT * FROM Acoustic").execute();
            Q("DROP TABLE Acoustic").execute();
        }
        if (from < 11)
        {
            Q("UPDATE Identify SET modtime = 0;").execute();
        }
        if (from < 12)
        {
            Q("CREATE TEMP TABLE Journal_backup "
                    "AS SELECT * FROM Journal;").execute();
            Q("DROP TABLE Journal;").execute();

            Q("CREATE TEMP TABLE Rating_backup "
                    "AS SELECT * FROM Rating;").execute();

            Q("DROP TABLE Rating;").execute();
            Q("DROP TABLE A.Acoustic;").execute();

            sql_create_tables();

            Q("INSERT INTO Ratings "
                    "SELECT uid, rating, 0 FROM Rating_backup").execute();

            string query =
                "INSERT INTO Journal ('uid', 'played', 'flags', 'time') "
                "SELECT uid, "
                "CASE "
                    "WHEN delta > 0 THEN 10 "
                    "ELSE 5 "
                "END, "
                "CASE"
                    " WHEN delta = -8 THEN " + itos(undeltify(-8)) +
                    " WHEN delta = -7 THEN " + itos(undeltify(-7)) +
                    " WHEN delta = -6 THEN " + itos(undeltify(-6)) +
                    " WHEN delta = -5 THEN " + itos(undeltify(-5)) +
                    " WHEN delta = -4 THEN " + itos(undeltify(-4)) + 
                    " WHEN delta = -1 THEN " + itos(undeltify(-1)) +
                    " WHEN delta = 1 THEN " + itos(undeltify(1)) +
                    " WHEN delta = 2 THEN " + itos(undeltify(2)) +
                    " WHEN delta = 3 THEN " + itos(undeltify(3)) +
                    " WHEN delta = 5 THEN " + itos(undeltify(5)) +
                    " WHEN delta = 6 THEN " + itos(undeltify(6)) +
                    " WHEN delta = 7 THEN " + itos(undeltify(7)) +
                    " WHEN delta = 8 THEN " + itos(undeltify(8)) +
                    " WHEN delta = 9 THEN " + itos(undeltify(9)) +
                " END, time "
                "FROM Journal_backup WHERE delta != 0;";

            Q(query).execute();
            Q("DROP TABLE Journal_backup;").execute();
        }
        if (from == 12) 
        {
            Q("DELETE FROM Bias WHERE mean = 33 and trials = 0;").execute();
            update_all_ratings();
        }

        a.commit();
    }
    WARNIFFAILED();

    if (from < 12)
        mass_rating_upgrade();
}

void BasicDb::fake_encounter(int uid, int delta, time_t time)
{
    if (!delta)
        return;

    Q q("INSERT INTO Journal ('uid', 'played', 'flags', 'time') "
            "VALUES (?, ?, ?, ?)");

    time_t played = delta > 0 ? 10 : 5;
    int flags = undeltify(delta);

    q << uid << played << flags << time << execute;
}

void BasicDb::mass_rating_upgrade()
{
    using std::max;
    using std::min;
    try
    {
        AutoTransaction at;
        time_t maxfirstseen = 0;

        Q q("SELECT avg(firstseen) FROM Library;");
        if (q.next() && q.not_null())
        {
            q >> maxfirstseen;
            maxfirstseen = (maxfirstseen - time(0)) * 2 / 5;
        }
        q.execute();

        Q select("SELECT Library.uid, rating, playcounter, firstseen "
                "FROM Library NATURAL JOIN Ratings;");
        Q insert("INSERT INTO Bias ('uid', 'mean', 'trials') "
                "VALUES (?, ?, ?);");

        int uid, rating, playcounter;
        time_t firstseen;
        int mean, trials;

        while (select.next())
        {
            select >> uid >> rating >> playcounter >> firstseen;
            mean = ROUND((rating - 75) * 100 / 75.0);
            firstseen -= time(0);
            int confidence = std::min(firstseen / maxfirstseen, 5L);
            confidence += std::min(playcounter, 5);
            trials = confidence * 5;

            if (!trials)
                continue;

            insert << uid << mean << trials;
            insert.execute();
        }

        at.commit();
    }
    WARNIFFAILED();

    update_all_ratings();
}

void BasicDb::update_all_ratings()
{
    try 
    {
        AutoTransaction at;

        Q q("SELECT DISTINCT uid FROM Library;");
        int uid;

        while (q.next())
        {
            q >> uid;
            Song song("", uid);
            song.update_rating();
        }

        Q("UPDATE Ratings SET rating = 50, dev = 16 "
                "WHERE rating > 100 or rating < 1").execute();

        at.commit();
    }
    WARNIFFAILED();
}
