#include <iostream>

#include "playlist.h"
#include "strmanip.h"
#include "utils.h"

using std::endl;
using std::cerr;

void PlaylistDb::sql_create_tables()
{
    RuntimeErrorBlocker reb;
    try {
        Q("CREATE TEMPORARY TABLE 'Playlist' ("
                "'pos' INTEGER PRIMARY KEY, "
                "'path' VARCHAR(4096) NOT NULL, "
                "'uid' INTEGER DEFAULT NULL, "
                "'ided' INTEGER DEFAULT '0');").execute();

        Q("CREATE TEMPORARY TABLE 'Matches' ("
                "'uid' INTEGER UNIQUE NOT NULL);").execute();

        Q("CREATE TEMPORARY VIEW 'Filter' AS "
                "SELECT pos FROM 'Playlist' WHERE Playlist.uid IN "
                "(SELECT uid FROM Matches)").execute();
    }
    WARNIFFAILED();
}

int PlaylistDb::install_filter(const string &filter)
{ 
    filtercount = -1;

    if (filter == "")
        return filtercount;

    try {
        Q("DELETE FROM 'Matches';").execute();
        {
            QueryCacheDisabler qcd;
            Q("INSERT INTO 'Matches' "
                    "SELECT DISTINCT(Library.uid) FROM 'Library' "
                    "INNER JOIN 'Rating' USING(uid) "
                    "LEFT OUTER JOIN 'Last' ON Last.sid = Library.sid "
                    "LEFT OUTER JOIN 'Acoustic' ON Acoustic.uid = Library.uid "
                    "LEFT OUTER JOIN 'Info' ON Info.sid = Library.sid "
                    "WHERE " + filter + ";").execute();
        }

        filtercount = -1;
        Q q("SELECT count(uid) FROM 'Matches';");
        if (q.next())
            q >> filtercount;
    }
    WARNIFFAILED();

    return filtercount;
}

int PlaylistDb::get_unknown_playlist_item()
{
    try {
        Q q("SELECT pos FROM 'Playlist' WHERE uid IS NULL LIMIT 1;");

        if (q.next())
        {
            int result;
            q >> result;
            return result;
        }
    }
    WARNIFFAILED();

#if 0
    select_query("SELECT pos FROM 'Playlist' WHERE ided = '0' LIMIT 1;");

    if (nrow && resultp[1])
        return atoi(resultp[1]);
#endif

    return -1;
}

bool PlaylistDb::playlist_id_from_item(int pos)
{
    try {
        Q q("SELECT Library.uid, Library.sid FROM 'Library' "
                "INNER JOIN 'Playlist' ON Library.uid = Playlist.uid "
                "WHERE Playlist.pos = ?;");
        q << pos;

        if (!q.next())
            return false;

        q >> uid >> sid;
        return true;
    }
    WARNIFFAILED();
    return false;
}

void PlaylistDb::playlist_update_identity(int pos)
{
    try {
        Q q("UPDATE 'Playlist' SET ided = '1', uid = ? WHERE pos = ?;");
        q << uid << pos;
        q.execute();
    }
    WARNIFFAILED();
}

void PlaylistDb::playlist_insert_item(int pos, const string &path)
{
    try {
        Q q("INSERT INTO 'Playlist' ('pos', 'path', 'uid') "
                "VALUES (?, ?, (SELECT uid FROM Library WHERE path = ?));");
        q << pos << path << path;
        q.execute();
    }
    WARNIFFAILED();
}

int PlaylistDb::get_effective_playlist_length()
{
    int result = 0;
    string table = filtercount > 0 ? "Filter" : "Playlist";

    try {
        Q q("SELECT count(pos) FROM " + table + ";");
        if (q.next())
            q >> result;
    }
    WARNIFFAILED();

    return result;
}

int PlaylistDb::random_playlist_position()
{
    string table = filtercount > 0 ? "Filter" : "Playlist";
    int result = -1;

    try {
        AutoTransaction a;
        int total = get_effective_playlist_length();

        {
            QueryCacheDisabler qcd;
            Q q("SELECT pos FROM " + table + " LIMIT 1 OFFSET "
                    + itos(imms_random(total)) + ";");

            if (q.next())
                q >> result;
        }
    }
    WARNIFFAILED();

    return result;
}

string PlaylistDb::get_playlist_item(int pos)
{
    string path;

    try {
        Q q("SELECT path FROM 'Playlist' WHERE pos = ?;");
        q << pos;
        if (q.next())
            q >> path;
    }
    WARNIFFAILED();
    
    return path;
}

void PlaylistDb::playlist_clear()
{
    try {
        Q("DELETE FROM 'Playlist';").execute();
    }
    WARNIFFAILED();
}
