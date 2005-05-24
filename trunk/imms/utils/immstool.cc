#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <list>
#include <utility>
#include <algorithm>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <imms.h>
#include <song.h>
#include <sqldb2.h>
#include <immsutil.h>
#include <strmanip.h>
#include <picker.h>
#include <appname.h>

#include <distance.h>
#include <beatkeeper.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::list;
using std::cin;
using std::setw;
using std::pair;
using std::ifstream;

const string AppName = IMMSTOOL_APP;

int usage();
int rating_usage();
void do_help();
void do_missing();
void do_purge(const string &path);
time_t get_last(const string &path);
void do_lint();
void do_distance(const string &path1, const string &path2);
void do_identify(const string &path);
int do_rate(const string &path, char *rating);

float EMD::cost[NUMGAUSS][NUMGAUSS];

int main(int argc, char *argv[])
{
    if (argc < 2)
        return usage();

    SqlDb sqldb;

    if (!strcmp(argv[1], "distance"))
    {
        if (argc != 4)
        {
            cout << "immstool distance path1 path2" << endl;
            return -1;
        }

        do_distance(argv[2], argv[3]);
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
    cout << " immstool distance|graph" << endl;
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

    r = min(MAX_RATING, max(MIN_RATING, r));

    cout << "New rating: " << r << endl;
    s.set_rating(r);
    return 0;
}

void do_distance(const string &path1, const string &path2)
{
    Song song1(path1), song2(path2);

    if (!song1.isok() || !song2.isok())
    {
        cerr << "could not identify '" << path1
            << "' or '" << path2 << "'" << endl;
        return;
    }

    MixtureModel m1, m2;
    float beats[BEATSSIZE];

    if (!song1.get_acoustic(&m1, sizeof(MixtureModel),
            beats, sizeof(beats)))
    {
        cerr << "loading acoustic data for song1" << endl;
        return;
    }

    if (!song2.get_acoustic(&m2, sizeof(MixtureModel),
            beats, sizeof(beats)))
    {
        cerr << "loading acoustic data for song2" << endl;
        return;
    }

    float distance = EMD::distance(m1, m2);
    cout << "Distance: " << distance << endl;
}
