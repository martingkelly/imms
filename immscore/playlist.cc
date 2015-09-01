/*
 IMMS: Intelligent Multimedia Management System
 Copyright (C) 2001-2009 Michael Grigoriev

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include <iostream>

#include "playlist.h"
#include "strmanip.h"
#include "immsutil.h" 

using std::endl;
using std::cerr;

void PlaylistDb::sql_create_tables()
{
    RuntimeErrorBlocker reb;
    try {
        Q("CREATE TABLE DiskPlaylist ("
                "'uid' INTEGER NOT NULL);").execute();

        Q("CREATE TABLE DiskMatches "
                "('uid' INTEGER UNIQUE NOT NULL);").execute();

        Q("CREATE TEMPORARY TABLE Playlist ("
                "'pos' INTEGER PRIMARY KEY, "
                "'path' VARCHAR(4096) NOT NULL, "
                "'uid' INTEGER DEFAULT -1);").execute();

        Q("CREATE TEMPORARY TABLE Matches "
                "('uid' INTEGER UNIQUE NOT NULL);").execute();

        Q("CREATE TEMPORARY VIEW Filter AS "
                "SELECT * FROM Playlist WHERE uid IN Matches "
                "OR NOT EXISTS (SELECT * FROM Matches LIMIT 1);").execute();
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
    }
    WARNIFFAILED();
}

void PlaylistDb::playlist_insert_item(int pos, const string &path)
{
    try {
        Q q("INSERT OR REPLACE INTO Playlist ('pos', 'path', 'uid') "
                "VALUES (?, ?, coalesce((SELECT uid FROM Identify "
                    "WHERE path = ?), -1));");
        q << pos << path << path;
        q.execute();
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
    if (effective_length_cache != -1)
        return effective_length_cache;

    try {
        Q q("SELECT count(1) FROM Filter WHERE uid != -2;");
        if (q.next())
            q >> effective_length_cache;
    }
    WARNIFFAILED();

    return effective_length_cache;
}

void PlaylistDb::get_random_sample(vector<int> &metacandidates, int size)
{
    try {
        int total = get_effective_playlist_length();

        Q q("SELECT pos FROM Filter "
                "WHERE uid != -2 AND (abs(random()) % ?) < ?;");
        q << total << (size + 5);

        int result;
        while (q.next())
        {
            q >> result;
            metacandidates.push_back(result);
        }

    }
    WARNIFFAILED();
}

void PlaylistDb::clear_matches()
{
    try {
        AutoTransaction a(AppName != IMMSD_APP);
        Q("DELETE FROM DiskMatches;").execute();
        a.commit();
    }
    IGNOREFAILURE();
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
        Q("DELETE FROM DiskPlaylist;").execute();
        Q("DELETE FROM DiskMatches;").execute();
    }
    WARNIFFAILED();
}

void PlaylistDb::sync()
{
    effective_length_cache = -1;
    try {
        AutoTransaction a;
        Q("DELETE FROM DiskPlaylist;").execute();
        Q("INSERT INTO DiskPlaylist "
                "SELECT uid FROM Playlist;").execute();
        Q("DELETE FROM Matches;").execute();
        Q("INSERT INTO Matches SELECT uid FROM DiskMatches;").execute();
        a.commit();
    }
    WARNIFFAILED();
}
