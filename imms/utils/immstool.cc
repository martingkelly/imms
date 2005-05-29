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

#include <imms.h>
#include <song.h>
#include <immsdb.h>
#include <immsutil.h>
#include <strmanip.h>
#include <picker.h>
#include <appname.h>

#include <classifier/distance.h>
#include <analyzer/beatkeeper.h>

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
time_t get_last(const string &path);
void do_closest(const string &path);
void do_lint();
void do_identify(const string &path);
int do_rate(const string &path, char *rating);
void do_update_distances();

int main(int argc, char *argv[])
{
    if (argc < 2)
        return usage();

    ImmsDb immsdb;

    if (!strcmp(argv[1], "distances"))
    {
        if (argc > 2)
        {
            cout << "huh??" << endl;
            return -1;
        }

        do_update_distances();
    }
    if (!strcmp(argv[1], "closest"))
    {
        if (argc != 3)
        {
            cout << "immstool closest <filename>" << endl;
            return -1;
        }

        do_closest(argv[2]);
    }
    else if (!strcmp(argv[1], "rate"))
    {
        if (argc < 4 || strlen(argv[2]) < 2)
            return rating_usage();

        for (int i = 3; i < argc; ++i)
            do_rate(argv[i], argv[2]);
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

        cutoff = time(0) - cutoff*24*60*60;

        string path;
        while (getline(cin, path))
        {
            if (get_last(path) < cutoff)
            {   
                do_purge(path);
                cout << " [X]";
            }
            else
                cout << " [_]"; 
            cout << " >> " << path_get_filename(path) << endl;
        }
        
        do_lint();
    }
    else if (!strcmp(argv[1], "graph"))
    {
        string str;
        if (argc > 1)
            str = argv[2];
        else
            cin >> str;
        for (unsigned i = 0; i < str.length(); ++i)
            cout << i << " " << (int)str[i] << endl;
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
    cout << " immstool rate|missing|purge|lint|identify|help" << endl;
    cout << "Debug functionality: " << endl;
    cout << " immstool distances|graph" << endl;
    return -1;
}

int rating_usage()
{
    cout << "immstool rate [+|-]<rating> <filename> [<filename>]*" << endl;
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

time_t get_last(const string &path)
{
    Q q("SELECT last FROM 'Last' "
                "INNER JOIN 'Library' ON Last.sid = Library.sid "
                "JOIN 'Identify' ON Identify.uid = Library.uid "
                "WHERE Identify.path = ?;");
    q << path;
    if (!q.next())
        return 0;

    time_t last;
    q >> last;
    return last;
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

        Q("DELETE FROM Rating "
                "WHERE uid NOT IN (SELECT uid FROM Library);").execute();

        Q("DELETE FROM A.Acoustic "
                "WHERE uid NOT IN (SELECT uid FROM Library);").execute();

        Q("DELETE FROM Info "
                "WHERE sid NOT IN (SELECT sid FROM Library);").execute();

        Q("DELETE FROM Artists "
                "WHERE aid NOT IN (SELECT aid FROM Info);").execute();

#ifdef DANGEROUS
        Q("DELETE FROM C.Correlations "
                "WHERE x NOT IN (SELECT sid FROM Library) "
                "OR y NOT IN (SELECT sid FROM Library);").execute();
#endif

        QueryCacheDisabler qcd;

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

int do_rate(const string &path, char *rating)
{
    Song s(path_normalize(path));

    if (!s.isok())
    {
        cerr << "Could not identify file: " << path << endl;
        return -1;
    }

    int r = s.get_rating();

    if (r < 0)
        r = 100;

    if (rating[0] == '-' || rating[0] == '+')
    {
        int mod = atoi(rating + 1);
        if (!mod)
            return rating_usage();
        r = r + (rating[0] == '-' ? - mod : mod);
    }
    else
    {
        r = atoi(rating);
        if (!r)
            return rating_usage();
    }

    r = std::min(MAX_RATING, std::max(MIN_RATING, r));

    cout << "New rating: " << r << endl;
    s.set_rating(r);
    return 0;
}

void do_update_distances()
{
    vector<int> uids;
    int uid;
    try
    {
        Q q("SELECT uid FROM A.AcousticNG WHERE mfcc NOTNULL;");
        while (q.next())
        {
            q >> uid;
            uids.push_back(uid);
        }
    }
    WARNIFFAILED();

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

        try {
            AutoTransaction at(true);
            Q q("INSERT OR REPLACE INTO A.Distances ('x', 'y', 'dist') "
                    "VALUES (?, ?, ?);");

            for (set<int>::iterator j = neigh.begin(); j != neigh.end(); ++j)
            {
                int small = std::min(uid, *j);
                int large = std::max(uid, *j);
                int dist = ROUND(song_cepstr_distance(uid, *j));
                if (dist < 0)
                    continue;
                if (dist > 255)
                {
                    cerr << "warning: bogus distance (" << dist << ") between "
                        << small << " and " << large << endl;
                    dist = 255;
                }
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
                "ORDER BY dist ASC LIMIT 25;");
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
