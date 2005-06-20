#include <iostream>

#include "playlist.h"
#include "strmanip.h"
#include "immsutil.h" 

#define UPDATE_DELAY    20

using std::endl;
using std::cerr;

void PlaylistDb::sql_create_tables()
{
    RuntimeErrorBlocker reb;
    try {
        Q("CREATE TEMPORARY TABLE Playlist ("
                "'pos' INTEGER PRIMARY KEY, "
                "'path' VARCHAR(4096) NOT NULL, "
                "'uid' INTEGER DEFAULT -1);").execute();

        Q("CREATE TEMPORARY TABLE Matches ("
                "'uid' INTEGER UNIQUE NOT NULL);").execute();

        Q("CREATE TEMPORARY VIEW Filter AS "
                "SELECT * FROM Playlist WHERE uid IN Matches").execute();
    }
    WARNIFFAILED();
}

int PlaylistDb::install_filter(const string &filter)
{ 
    string old_filter = filter_clause;
    filter_clause = filter;

    update_filter();

    int l = get_effective_playlist_length();
    if (l)
        return l;

    filter_clause = old_filter;
    update_filter();
    return -1;
}

void PlaylistDb::update_filter()
{
    string where_clause = (filter_clause == "") ? "1" : filter_clause;

    filter_update_requested = 0;

    try {
        Q("DELETE FROM Matches;").execute();

        QueryCacheDisabler qcd;
        Q("INSERT INTO Matches "
                "SELECT DISTINCT(P.uid) FROM Playlist P "
                "INNER JOIN Library L USING(uid) "
                "INNER JOIN Ratings USING(uid) "
                "LEFT OUTER JOIN Info I ON I.sid = L.sid "
                "WHERE " + where_clause + ";").execute();
    }
    WARNIFFAILED();
}

int PlaylistDb::get_unknown_playlist_item()
{
    try {
        Q q("SELECT pos FROM Playlist WHERE uid = -1 LIMIT 1;");

        if (q.next())
        {
            int result;
            q >> result;
            return result;
        }
    }
    WARNIFFAILED();

    return -1;
}

Song PlaylistDb::playlist_id_from_item(int pos)
{
    try {
        Q q("SELECT L.uid, L.sid, P.path FROM Library L "
                "INNER JOIN Playlist P USING(uid) WHERE P.pos = ?;");
        q << pos;

        if (!q.next())
            return Song();

        int uid, sid;
        string path;
        q >> uid >> sid >> path;
        return Song(path, uid, sid);
    }
    WARNIFFAILED();
    return Song();
}

void PlaylistDb::playlist_update_identity(int pos, int uid)
{
    try {
        Q q("UPDATE Playlist SET uid = ? WHERE pos = ?;");
        q << uid << pos;
        q.execute();

        filter_update_requested = UPDATE_DELAY;
    }
    WARNIFFAILED();
}

void PlaylistDb::do_events()
{
    if (filter_update_requested && !--filter_update_requested)
        update_filter();
}

void PlaylistDb::playlist_insert_item(int pos, const string &path)
{
    try {
        Q q("INSERT OR REPLACE INTO Playlist ('pos', 'path', 'uid') "
                "VALUES (?, ?, "
                "ifnull((SELECT uid FROM Identify WHERE path = ?), -1));");
        q << pos << path << path;
        q.execute();

        filter_update_requested = UPDATE_DELAY;
    }
    WARNIFFAILED();
}

int PlaylistDb::get_real_playlist_length()
{
    int result = 0;
    try {
        Q q("SELECT count(1) FROM Playlist;");
        if (q.next())
            q >> result;
    }
    WARNIFFAILED();

    return result;
}

int PlaylistDb::get_effective_playlist_length()
{
    int result = 0;
    try {
        Q q("SELECT count(1) FROM Filter;");
        if (q.next())
            q >> result;
    }
    WARNIFFAILED();

    return result;
}

int PlaylistDb::random_playlist_position()
{
    int result = -1;

    try {
        AutoTransaction a;
        int total = get_effective_playlist_length();

        Q q("SELECT pos FROM Filter LIMIT 1 OFFSET ?;");
        q << imms_random(total);

        if (q.next())
            q >> result;
    }
    WARNIFFAILED();

    return result;
}

string PlaylistDb::get_item_from_playlist(int pos)
{
    string path;

    try {
        Q q("SELECT path FROM Playlist WHERE pos = ?;");
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
        Q("DELETE FROM Playlist;").execute();
        Q("DELETE FROM Matches;").execute();
    }
    WARNIFFAILED();
}
