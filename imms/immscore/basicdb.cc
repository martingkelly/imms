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

        Q("CREATE INDEX Jouranl_uid_i "
                "ON Journal (uid);").execute();
    }
    WARNIFFAILED();
}

void update_rating(int uid);

int BasicDb::avg_rating(const string &artist, const string &title)
{
    try
    {
        if (title != "")
        {
            Q q("SELECT avg(rating) FROM Library AS L "
                    "INNER JOIN Info AS I ON L.sid = I.sid "
                    "INNER JOIN Ratings AS R ON L.uid = R.uid "
                    "INNER JOIN Artists AS A ON I.aid = A.aid "
                    "WHERE A.artist = ? AND I.title = ?;");

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
            Q q("SELECT avg(rating) FROM Library AS L"
                    "INNER JOIN Info AS I on L.sid = I.sid "
                    "INNER JOIN Ratings AS R ON R.uid = L.uid "
                    "INNER JOIN Artists AS A ON I.aid = A.aid "
                    "WHERE A.artist = ?;");
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

int BasicDb::avg_playcounter()
{
    static int playcounter = -1;
    if (playcounter != -1)
        return playcounter;

    try
    {
        Q q("SELECT avg(playcounter) from Library;");
        if (q.next())
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

        a.commit();
    }
    WARNIFFAILED();

    if (from < 12)
        mass_rating_update();
}

void fake_encounter(int uid, int delta, time_t time)
{
    Q q("INSERT INTO Journal ('uid', 'played', 'flags', 'time') "
            "VALUES (?, ?, ?, ?)");

    time_t played = delta > 0 ? 10 : 5;
    int flags = undeltify(delta);

    q << uid << played << flags << time << execute;
}

void BasicDb::mass_rating_update()
{
    try
    {
        AutoTransaction at;

        Q q("SELECT L.uid, playcounter FROM Library as L NATURAL JOIN Ratings "
                "WHERE rating <= ? AND RATING > ?;");

        time_t start_at = time(0) - 30*DAY;
        time_t current = start_at;

        int uid, playcounter;

        q << 250 << 140;

        while (q.next())
        {
            current = start_at;
            q >> uid >> playcounter;
            for (int i = 0; i < std::min(playcounter, 5); ++i)
            {
                fake_encounter(uid, +9, --current);
                fake_encounter(uid, +1, --current);
            }
        }

        q << 140 << 130;

        while (q.next())
        {
            current = start_at;
            q >> uid >> playcounter;
            for (int i = 0; i < std::min(playcounter, 5); ++i)
            {
                fake_encounter(uid, +9, --current);
                fake_encounter(uid, -1, --current);
            }
        }

        q << 130 << 120;

        while (q.next())
        {
            current = start_at;
            q >> uid >> playcounter;
            for (int i = 0; i < std::min(playcounter, 5); ++i)
            {
                fake_encounter(uid, +8, --current);
                fake_encounter(uid, -1, --current);
                fake_encounter(uid, -1, --current);
            }
        }

        q << 120 << 110;

        while (q.next())
        {
            current = start_at;
            q >> uid >> playcounter;
            for (int i = 0; i < std::min(playcounter, 5); ++i)
            {
                fake_encounter(uid, +7, --current);
                fake_encounter(uid, -1, --current);
                fake_encounter(uid, -1, --current);
                fake_encounter(uid, -1, --current);
            }
        }

        q << 110 << 100;

        while (q.next())
        {
            current = start_at;
            q >> uid >> playcounter;
            for (int i = 0; i < std::min(playcounter, 5); ++i)
            {
                fake_encounter(uid, +6, --current);
                fake_encounter(uid, -4, --current);
            }
        }

        q << 100 << 90;

        while (q.next())
        {
            current = start_at;
            q >> uid >> playcounter;
            for (int i = 0; i < std::min(playcounter, 5); ++i)
            {
                fake_encounter(uid, +5, --current);
                fake_encounter(uid, -4, --current);
                fake_encounter(uid, -1, --current);
            }
        }

        q << 90 << 80;

        while (q.next())
        {
            current = start_at;
            q >> uid >> playcounter;
            for (int i = 0; i < std::min(playcounter, 5); ++i)
            {
                fake_encounter(uid, -6, --current);
                fake_encounter(uid, +3, --current);
                fake_encounter(uid, +1, --current);
            }
        }

        q << 80 << 0;

        while (q.next())
        {
            current = start_at;
            q >> uid >> playcounter;
            for (int i = 0; i < std::min(playcounter, 5); ++i)
            {
                fake_encounter(uid, -8, --current);
                fake_encounter(uid, +2, --current);
            }
        }

        at.commit();
    }
    WARNIFFAILED();

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

        Q("UPDATE Ratings SET rating = 50, dev = 10 "
                "WHERE rating > 100 or rating < 1").execute();

        at.commit();
    }
    WARNIFFAILED();
}
