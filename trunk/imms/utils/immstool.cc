#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <list>
#include <utility>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <imms.h>
#include <song.h>
#include <sqldb2.h>
#include <utils.h>
#include <spectrum.h>
#include <strmanip.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::list;
using std::cin;
using std::setw;
using std::pair;
using std::ifstream;

int usage();
void do_help();
void do_deviation();
void do_missing();
void do_purge(const string &path);
time_t get_last(const string &path);
void do_lint();
void do_dump_bpm();
void do_spec_distance(const string &to);
void do_bpm_distance(const string &to);
void do_identify(const string &path);

int main(int argc, char *argv[])
{
    if (argc < 2)
        return usage();

    SqlDb sqldb;

    if (!strcmp(argv[1], "dumpbpm"))
    {
        do_dump_bpm();
    }
    else if (!strcmp(argv[1], "bpmdistance"))
    {
        if (argc < 3)
        {
            cout << "immstool distance <reference bpm>" << endl;
            return -1;
        }

        do_bpm_distance(argv[2]);
    }
    else if (!strcmp(argv[1], "specdistance"))
    {
        if (argc < 3)
        {
            cout << "immstool distance <reference spectrum>" << endl;
            return -1;
        }

        do_spec_distance(argv[2]);
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
    else if (!strcmp(argv[1], "deviation"))
    {
        do_deviation();
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
    cout << " immstool missing|purge|lint|identify|help" << endl;
    cout << "Debug functionality: " << endl;
    cout << " immstool bpmdistance|specdistance|deviation|graph" << endl;
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

string sid2info(int sid)
{
    Q q("SELECT artist, title FROM Info WHERE sid = ?");
    q << sid;

    if (!q.next())
        return "??";

    string artist, title;
    q >> artist >> title;

    return artist + " / " + title;
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

    Q q("SELECT x, y, weight FROM Correlations "
            "WHERE (x = ? OR y = ?) AND weight > 1 ORDER BY (weight) DESC");
    q << s.get_sid() << s.get_sid();

    while (q.next())
    {
        int x, y, weight, other;
        q >> x >> y >> weight;
        other = (x == s.get_sid()) ? y : x;

        cout << setw(8) << other << " (" << weight << ") - "
            << sid2info(other) << endl;
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

        Q("DELETE FROM Acoustic "
                "WHERE uid NOT IN (SELECT uid FROM Library);").execute();

#ifdef DANGEROUS
        Q("DELETE FROM Info "
                "WHERE sid NOT IN (SELECT sid FROM Library);").execute();

        Q("DELETE FROM Correlations "
                "WHERE x NOT IN (SELECT sid FROM Library) "
                "OR y NOT IN (SELECT sid FROM Library);").execute();
#endif

        QueryCacheDisabler qcd;

        Q("VACUUM Identify;").execute();
        Q("VACUUM Library;").execute();
        Q("VACUUM Correlations;").execute();
    }
    WARNIFFAILED();

}

void do_bpm_distance(const string &to)
{
    string rescaled = rescale_bpmgraph(to);

    Q q("SELECT Identify.path, Acoustic.bpm, Library.sid FROM 'Library' "
            "INNER JOIN 'Acoustic' ON Library.uid = Acoustic.uid "
            "JOIN 'Identify' ON Identify.uid = Library.uid "
            "WHERE Acoustic.bpm NOT NULL;");

    while(q.next())
    {
        string path, bpm;
        int sid;
        q >> path >> bpm >> sid;

        cout << setw(4) << rms_string_distance(rescaled, rescale_bpmgraph(bpm))
            << "  " << bpm << "  ";

        Q q("SELECT artist, title FROM Info WHERE sid = ?;");
        q << sid;

        if (q.next())
        {
            string artist, title;
            q >> artist >> title;
            cout << setw(25) << artist;
            cout << setw(25) << title;
        }
        else
            cout << setw(50) << path_get_filename(path);
        cout << endl;
    }
}

void do_spec_distance(const string &to)
{
    Q q("SELECT Identify.path, Acoustic.spectrum, Library.sid FROM 'Library' "
            "INNER JOIN 'Acoustic' ON Library.uid = Acoustic.uid "
            "JOIN 'Identify' ON Identify.uid = Library.uid "
            "WHERE Acoustic.spectrum NOT NULL;");

    while(q.next())
    {
        string path, spectrum;
        int sid;
        q >> path >> spectrum >> sid;

        cout << setw(4) << rms_string_distance(to, spectrum, 15)
            << "  " << spectrum << "  ";

        Q q("SELECT artist, title FROM Info WHERE sid = ?;");
        q << sid;

        if (q.next())
        {
            string artist, title;
            q >> artist >> title;
            cout << setw(25) << artist;
            cout << setw(25) << title;
        }
        else
            cout << setw(50) << path_get_filename(path);
        cout << endl;
    }
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

void do_deviation()
{
    list<string> all;
    string spectrum;
    int count = 0;
    double mean[SHORTSPECTRUM];
    memset(&mean, 0, sizeof(mean));
    while (cin >> spectrum)
    {
        if ((int)spectrum.length() != SHORTSPECTRUM)
        {
            cout << "bad spectrum: " << spectrum << endl;
            continue;
        }
        ++count;
        all.push_back(spectrum);
        for (int i = 0; i < SHORTSPECTRUM; ++i)
            mean[i] += (spectrum[i] - 'a');
    }

    // total to mean
    for (int i = 0; i < SHORTSPECTRUM; ++i)
        mean[i] /= count;

    cout << "Mean     : ";
    for (int i = 0; i < SHORTSPECTRUM; ++i)
        cout << std::setw(4) << ROUND(mean[i] * 10);
    cout << endl;

    double deviations[SHORTSPECTRUM];
    memset(&deviations, 0, sizeof(deviations));

    for (list<string>::iterator i = all.begin(); i != all.end(); ++i)
        for (int j = 0; j < SHORTSPECTRUM; ++j)
            deviations[j] += pow(mean[j] + 'a' - (*i)[j], 2);

    for (int i = 0; i < SHORTSPECTRUM; ++i)
        deviations[i] = sqrt(deviations[i] / count);

    cout << "Deviation : ";
    for (int i = 0; i < SHORTSPECTRUM; ++i)
        cout << std::setw(4) << ROUND(deviations[i] * 10);
    cout << endl;
}

void do_dump_bpm()
{
    Q q("SELECT uid, bpm, spectrum FROM Acoustic;");

    bool first = true;
    unsigned len = 0;

    while (q.next())
    {
        int uid = 0;
        string bpm, spectrum;
        q >> uid >> bpm >> spectrum;

        string point = rescale_bpmgraph(bpm) + rescale_bpmgraph(spectrum);

        if (first)
        {
            len = point.length();
            cout << len << endl;
            first = false;
        }

        if (point.length() != len)
            continue;

        for (unsigned i = 0; i < point.length(); ++i)
            cout << point[i] - 'a' << " ";
        cout << uid << endl;
    }
}
