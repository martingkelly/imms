#include <iostream>

#include "playlist.h"
#include "strmanip.h"
#include "utils.h"

using std::endl;
using std::cerr;

void PlaylistDb::sql_create_tables()
{
    run_query(
        "CREATE TEMPORARY TABLE 'Playlist' ("
            "'pos' INTEGER PRIMARY KEY, "
            "'path' VARCHAR(4096) NOT NULL, "
            "'uid' INTEGER DEFAULT NULL, "
            "'ided' INTEGER DEFAULT '0');");

    run_query(
        "CREATE TEMPORARY TABLE 'Matches' ("
            "'uid' INTEGER UNIQUE NOT NULL);");

    run_query(
        "CREATE TEMPORARY VIEW 'Filter' AS "
            "SELECT pos FROM 'Playlist' WHERE Playlist.uid IN "
            "(SELECT uid FROM Matches)");
}

int PlaylistDb::install_filter(const string &filter)
{
    if (filter == "")
        return filtercount = -1;

    run_query("DELETE FROM 'Matches';");
    run_query("INSERT INTO 'Matches' "
            "SELECT DISTINCT(Library.uid) FROM 'Library' "
                "INNER JOIN 'Rating' ON Rating.uid = Library.uid "
                "OUTER JOIN 'Acoustic' ON Acoustic.uid = Library.uid "
                "INNER JOIN 'Last' ON Last.sid = Library.sid "
                "INNER JOIN 'Info' ON Info.sid = Library.sid "
                    "WHERE " + filter + ";");

    select_query("SELECT count(uid) FROM 'Matches';");
    filtercount = nrow && resultp[1] ? atoi(resultp[1]) : -1;
    return filtercount;
}

int PlaylistDb::get_unknown_playlist_item()
{
    select_query("SELECT pos FROM 'Playlist' WHERE uid IS NULL LIMIT 1;");

    if (nrow && resultp[1])
        return atoi(resultp[1]);
#if 0
    select_query("SELECT pos FROM 'Playlist' WHERE ided = '0' LIMIT 1;");

    if (nrow && resultp[1])
        return atoi(resultp[1]);
#endif

    return -1;
}

bool PlaylistDb::playlist_id_from_item(int pos)
{
    select_query("SELECT Library.uid, Library.sid FROM 'Library' "
           "INNER JOIN 'Playlist' ON Library.uid = Playlist.uid "
           "WHERE Playlist.pos = '" + itos(pos) + "';");

    if (!nrow)
        return false;

    uid = atoi(resultp[ncol]);
    sid = atoi(resultp[ncol + 1]);
    return true;
}

void PlaylistDb::playlist_update_identity(int pos)
{
    run_query("UPDATE 'Playlist' SET ided = '1', uid = '" + itos(uid) + "' "
            "WHERE pos = '" + itos(pos) + "';");
}

void PlaylistDb::playlist_insert_item(int pos, const string &path)
{
    string epath = escape_string(path);
    run_query("INSERT INTO 'Playlist' ('pos', 'path', 'uid') "
            "VALUES ('" + itos(pos) + "', '" + epath + "', "
                "(SELECT uid FROM Library WHERE path = '" + epath + "'));");
}

int PlaylistDb::random_playlist_position()
{
    string table = filtercount > 0 ? "Filter" : "Playlist";
    select_query("SELECT count(pos) FROM " + table + ";");
    int total = nrow && resultp[1] ? atoi(resultp[1]) : 0;

    int num = imms_random(total);
    select_query(
            "SELECT pos FROM " + table
            + " LIMIT 1 OFFSET " + itos(num) + ";");

    return nrow && resultp[1] ? atoi(resultp[1]) : -1;
}

string PlaylistDb::get_playlist_item(int pos)
{
    select_query("SELECT path FROM 'Playlist' "
            "WHERE pos = '" + itos(pos) + "';");

    return nrow && resultp[1] ? resultp[1] : "";
}

void PlaylistDb::playlist_clear()
{
    run_query("DELETE FROM 'Playlist';");
}
