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
        Q("PRAGMA cache_size = 5000").execute();
        Q("PRAGMA temp_store = MEMORY;").execute();
    }
    WARNIFFAILED();
}

void BasicDb::sql_create_tables()
{
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
    try 
    {
        AutoTransaction a;
        if (from < 14)
        {
            Q("DROP TABLE A.Acoustic;").execute();
        }

        a.commit();
    }
    IGNOREFAILURE();  // Temporary hack to work around broken schema upgrades.
}
