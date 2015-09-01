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
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <list>
#include <set>
#include <map>
#include <utility>
#include <algorithm>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <immsconf.h>
#include <imms.h>
#include <song.h>
#include <immsdb.h>
#include <immsutil.h>
#include <strmanip.h>
#include <picker.h>
#include <appname.h>
#include <string.h>

#include <analyzer/beatkeeper.h>
#include <analyzer/mfcckeeper.h>
#include <model/distance.h>
#include <model/model.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::list;
using std::cin;
using std::setw;
using std::pair;
using std::ifstream;
using std::set;
using std::multimap;

const string AppName = IMMSTOOL_APP;

int usage();
int rating_usage();
void do_help();
void do_missing();
void do_purge(const string &path);
void do_closest(const string &path);
void do_lint();
void do_identify(const string &path);
void do_update_ratings();
void do_update_distances();

int main(int argc, char *argv[])
{
    LOG(ERROR) << string(60, '*') << endl;
    LOG(ERROR) << "*   this utility is completely unsupported" << endl;
    LOG(ERROR) << "*   please make sure to backup of your .imms directory" << endl;
    LOG(ERROR) << string(60, '*') << endl;

    if (argc < 2)
        return usage();

    ImmsDb immsdb;

    if (!strcmp(argv[1], "ratings"))
    {
        if (argc > 2)
        {
            cout << "huh??" << endl;
            return -1;
        }

        do_update_ratings();
    }
    else if (!strcmp(argv[1], "distances"))
    {
        if (argc > 2)
        {
            cout << "huh??" << endl;
            return -1;
        }

        do_update_distances();
    }
    else if (!strcmp(argv[1], "distance"))
    {
        if (argc != 4)
        {
            cout << "huh??" << endl;
            return -1;
        }

        Song s1(argv[2]), s2(argv[3]);

        if (!s1.isok() || !s2.isok())
        {
            LOG(ERROR) << "Failed to id songs" << endl;
            return -2;
        }

        SVMSimilarityModel m;
        LOG(INFO) << "Distance = " << m.evaluate(s1, s2) << endl;;
    }
    else if (!strcmp(argv[1], "filter"))
    {
        int cutoff = 75;
        if (argc > 2)
            cutoff = atoi(argv[2]);
        if (cutoff < 1)
            return -1;
        LOG(ERROR) << "Filtering at cutoff = " << cutoff << endl;
        string filename;
        while (getline(cin, filename))
        {
            Song s(path_normalize(filename));
            if (!s.isok())
                continue;
            int rating = s.get_rating();
            if (rating >= cutoff)
                cout << s.get_sid() << ":" << filename << endl;
        }
    }
    else if (!strcmp(argv[1], "closest"))
    {
        if (argc != 3)
        {
            cout << "immstool closest <filename>" << endl;
            return -1;
        }

        do_closest(argv[2]);
    }
    else if (!strcmp(argv[1], "identify"))
    {
        if (argc < 3)
        {
            cout << "immstool identify <filename>" << endl;
            return -1;
        }

        do_identify(argv[2]);
    }
    else if (!strcmp(argv[1], "missing"))
    {
        do_missing();
    }
    else if (!strcmp(argv[1], "purge"))
    {
        time_t cutoff = 30;
        if (argc == 3)
            cutoff = atoi(argv[2]);

        if (!cutoff)
        {
            cout << "immstool purge [n days]" << endl;
            return -1;
        }


        string path;
        vector<string> paths;
        while (getline(cin, path))
            paths.push_back(path);

        time_t lastseen;
        cutoff = time(0) - cutoff*24*60*60;

        try
        {
            Q q("SELECT lastseen FROM Library NATURAL JOIN Identify "
                    "WHERE path = ?;");

            for (unsigned i = 0; i < paths.size(); ++i)
            {
                q << paths[i];
                if (!q.next())
                    continue;

                q >> lastseen;
                q.execute();

                if (lastseen < cutoff)
                {   
                    do_purge(paths[i]);
                    cout << " [X]";
                }
                else
                    cout << " [_]"; 
                cout << " >> " << path_get_filename(paths[i]) << endl;
            }
        }
        WARNIFFAILED();
    }
    else if (!strcmp(argv[1], "graph"))
    {
        if (argc < 3)
        {
            cout << "immstool graph <filename>" << endl;
            return -1;
        }

        Song s(path_normalize(argv[2]));

        if (!s.isok())
        {
            cerr << "identify failed!" << endl;
            cout << "path       : " << s.get_path() << endl;
            return -1;
        }

        MixtureModel m;
        float beats[BEATSSIZE];

        if (!s.get_acoustic(&m, beats))
        {
            LOG(ERROR) << "failed to load acoustic data for file "
                << argv[2] << endl;
            return -1;
        }

        for (unsigned i = 0; i < BEATSSIZE; ++i)
            cout << OFFSET2BPM(i) << " " << beats[i] << endl;
        return 0;
    }
    else if (!strcmp(argv[1], "lint"))
    {
        do_lint();
    }
    else if (!strcmp(argv[1], "help"))
    {
        do_help();
    }
    else
        return usage();

    return 0;
}

int usage()
{
    cout << "End user functionality: " << endl;
    cout << " immstool missing|purge|lint|identify|help" << endl;
    cout << "Debug functionality: " << endl;
    cout << " immstool distances|graph" << endl;
    return -1;
}

void do_help()
{
    cout << "immstool <command> <arguments>" << endl;
    cout << "  Available commands are:" << endl;
    cout << "    missing                " <<
        "- list missing files" << endl;
    cout << "    purge [n]              " <<
        "- remove from database if last played more than n days ago" << endl;
    cout << "                           " << 
        "  hint: 'immstool missing | sort | immstool purge' works well" << endl;
    cout << "    lint                   " <<
        "- vacuum the database" << endl;
    cout << "    identify <filename>    " <<
        "- print information about a given file" << endl;
    cout << "    help                   " << 
        "- show this help" << endl;
}

StringPair sid2info(int sid)
{
    Q q("SELECT artist, title FROM Info "
           "NATURAL INNER JOIN Artists WHERE sid = ?");
    q << sid;

    StringPair info;
    if (q.next())
        q >> info.first >> info.second;

    return info;
}

string sid2string(int sid)
{
    StringPair p = sid2info(sid);
    return p.first + " / " + p.second;
}

void do_identify(const string &path)
{
    Song s(path_normalize(path));

    if (!s.isok())
    {
        cerr << "identify failed!" << endl;
        cout << "path       : " << s.get_path() << endl;
        exit(-1);
    } 

    s.update_rating();

    cout << "path       : " << s.get_path() << endl;
    cout << "uid        : " << s.get_uid() << endl;
    cout << "sid        : " << s.get_sid() << endl;
    cout << "artist     : " << s.get_info().first << endl;
    cout << "title      : " << s.get_info().second << endl;
    cout << "rating     : " << s.get_rating() << endl;
    cout << "last       : " << strtime(time(0) - s.get_last()) << endl;

    cout << endl << "positively correlated with: " << endl;

    Q q("SELECT x, y, weight FROM C.Correlations "
            "WHERE (x = ? OR y = ?) AND weight > 1 ORDER BY (weight) DESC");
    q << s.get_sid() << s.get_sid();

    while (q.next())
    {
        int x, y, weight, other;
        q >> x >> y >> weight;
        other = (x == s.get_sid()) ? y : x;

        cout << setw(8) << other << " (" << weight << ") - "
            << sid2string(other) << endl;
    }
}

void do_purge(const string &path)
{
    Q q("DELETE FROM 'Identify' WHERE path = ?;");
    q << path;
    q.execute();
}

void do_lint()
{
    try
    {
        {
            AutoTransaction at;
            vector<string> deleteme;
            vector<string> cleanme;

            {
                Q q("SELECT path FROM Identify;");
                while (q.next())
                {
                    string path;
                    q >> path;
                    string simple = path_normalize(path);

                    if (path == simple)
                        continue;

                    Q q("SELECT path FROM Identify WHERE path = ?");
                    q << simple;

                    if (q.next())
                        deleteme.push_back(path);
                    else
                        cleanme.push_back(path);
                }
            }

            cout << "Duplicates: " << endl;

            for (vector<string>::iterator i = deleteme.begin(); 
                    i != deleteme.end(); ++i)
            {
                Q q("DELETE FROM Identify WHERE path = ?");
                q << *i;
                q.execute();
                cout << *i << endl;
            }

            cout << "Non-normalized: " << endl;

            for (vector<string>::iterator i = cleanme.begin(); 
                    i != cleanme.end(); ++i)
            {
                Q q("UPDATE Identify SET path = ? WHERE path = ?");
                q << path_normalize(*i) << *i;
                q.execute();
                cout << *i << endl;
            }

            at.commit();
        }

        Q("DELETE FROM Library "
                "WHERE uid NOT IN (SELECT uid FROM Identify);").execute();

        Q("DELETE FROM Last "
                "WHERE sid NOT IN (SELECT sid FROM Library);").execute();

        Q("DELETE FROM Ratings "
                "WHERE uid NOT IN (SELECT uid FROM Library);").execute();

        Q("DELETE FROM A.Acoustic "
                "WHERE uid NOT IN (SELECT uid FROM Library);").execute();

        Q("DELETE FROM Info "
                "WHERE sid NOT IN (SELECT sid FROM Library);").execute();

        Q("DELETE FROM Artists "
                "WHERE aid NOT IN (SELECT aid FROM Info);").execute();

        Q("DELETE FROM C.Correlations "
                "WHERE x NOT IN (SELECT sid FROM Library) "
                "OR y NOT IN (SELECT sid FROM Library);").execute();

        Q("VACUUM Identify;").execute();
        Q("VACUUM Library;").execute();
    }
    WARNIFFAILED();

}

void do_missing()
{
    Q q("SELECT path FROM 'Identify';");

    while (q.next())
    {
        string path;
        q >> path;
        if (access(path.c_str(), F_OK))
            cout << path << endl;
    }
}

void do_update_ratings()
{
    vector<int> uids;
    try
    {
        Q q("SELECT DISTINCT uid FROM Journal;");
        while (q.next())
        {
            int uid;
            q >> uid;
            uids.push_back(uid);
        }
    }
    WARNIFFAILED();

    for (size_t i = 0; i < uids.size(); ++i)
    {
        Song song("", uids[i]);
        song.update_rating();
    }
}

void do_update_distances()
{
    vector<int> uids;
    try
    {
        Q q("SELECT uid FROM A.Acoustic WHERE mfcc NOTNULL AND bpm NOTNULL;");
        while (q.next())
        {
            int uid;
            q >> uid;
            uids.push_back(uid);
        }
    }
    WARNIFFAILED();

    SVMSimilarityModel model;

    size_t numuids = uids.size();
    for (size_t i = 0; i < numuids; ++i)
    {
        int uid = uids[i];
        set<int> neigh;
        neigh.insert(uids.begin(), uids.end());
        neigh.erase(uid);
        int x, y;

        try
        {
            Q q("SELECT x, y FROM A.Distances WHERE x = ? OR y = ?;");
            q << uid << uid;
            while (q.next()) 
            {
                q >> x >> y;
                if (x == uid && y != uid)
                    neigh.erase(y);
                if (x != uid && y == uid)
                    neigh.erase(x);
            }
        }
        WARNIFFAILED();

        MixtureModel m1;
        float beats1[BEATSSIZE];

        Song s1("", uid);
        if (!s1.get_acoustic(&m1, beats1))
            continue;

        try {
            AutoTransaction at(true);
            Q q("INSERT OR REPLACE INTO A.Distances ('x', 'y', 'dist') "
                    "VALUES (?, ?, ?);");

            for (set<int>::iterator j = neigh.begin(); j != neigh.end(); ++j)
            {
                MixtureModel m2;
                float beats2[BEATSSIZE];

                Song s2("", *j);
                if (!s2.get_acoustic(&m2, beats2))
                    continue;

                int small = std::min(uid, *j);
                int large = std::max(uid, *j);

                int dist = ROUND(model.evaluate(m1, beats1, m2, beats2) * 100);

                // Don't bother with distance < 0.3.
                // This way we only get a list of strongly correlated songs.
                if (dist < 30)
                    continue;

                cout << "updating " << small << " -> "
                    << large << " = " << dist << endl;
                q << small << large << dist;
                q.execute();
            }
            at.commit();
        }
        WARNIFFAILED();
    }
}

void do_closest(const string &path)
{
    Song song(path);

    if (!song.isok())
    {
        cerr << "immstool: could not identify " << path << endl;
        return; 
    }
    
    int uid = song.get_uid();

    multimap<int, int> closest;

    try
    {
        Q q("SELECT x,y,dist FROM A.Distances WHERE x = ? or y = ? "
                "ORDER BY dist DESC LIMIT 25;");
        q << uid << uid;

        int x, y, dist;
        while (q.next())
        {
            q >> x >> y >> dist;
            int other = -1;
            if (x == uid && y != uid)
                other = y;
            if (x != uid && y == uid)
                other = x;
            if (other > 0)
                closest.insert(pair<int, int>(dist, other));
        }
    }
    WARNIFFAILED();

    try 
    {
        Q q("SELECT path FROM Identify WHERE uid = ?;");
        for (multimap<int, int>::iterator i = closest.begin();
                i != closest.end(); ++i)
        {
            q << i->second;
            if (q.next())
            {
                string path;
                q >> path;
                cout << i->first << ": " << path_get_filename(path) << endl;
            }
            q.execute();
        }
    }
    WARNIFFAILED();
}
