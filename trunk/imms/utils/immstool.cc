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
#include <sqldb2.h>
#include <utils.h>
#include <player.h>
#include <spectrum.h>
#include <strmanip.h>

using std::string;
using std::cout;
using std::endl;
using std::list;
using std::cin;
using std::setw;
using std::pair;
using std::ifstream;

int Player::get_playlist_length() { return 0; }
string Player::get_playlist_item(int i) { return ""; }

int g_argc;
char **g_argv;

class ImmsTool : public SqlDb
{
public:
    ImmsTool()
        : SqlDb(string(getenv("HOME")).append("/.imms/imms.db")) { }

    void do_distance();
    int distance_callback(const string &path,
            const string &spectrum, int sid);
    void do_missing();
    time_t get_last(const string &path);
    void do_purge(const string &path);
    void do_lint();
};

int usage();
void do_help();
void do_deviation();
void do_deviation_2(const string &filename);

int main(int argc, char *argv[])
{
    if (argc < 2)
        return usage();

    g_argv = argv;
    g_argc = argc;

    ImmsTool immstool;

    if (!strcmp(argv[1], "distance"))
    {
        if (argc < 3)
        {
            cout << "immstool distance <reference spectrum>" << endl;
            return -1;
        }

        immstool.do_distance();
    }
    else if (!strcmp(argv[1], "deviation"))
    {
        do_deviation();
    }
    else if (!strcmp(argv[1], "deviation2"))
    {
        if (argc < 3)
            return usage();
        do_deviation_2(argv[2]);
    }
    else if (!strcmp(argv[1], "missing"))
    {
        immstool.do_missing();
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
            if (immstool.get_last(path) < cutoff)
            {   
                immstool.do_purge(path);
                cout << " [X]";
            }
            else
                cout << " [_]"; 
            cout << " >> " << path_get_filename(path) << endl;
        }
        
        immstool.do_lint();
    }
    else if (!strcmp(argv[1], "lint"))
    {
        immstool.do_lint();
    }
    else if (!strcmp(argv[1], "help"))
    {
        do_help();
    }
    else
        return usage();
}

int usage()
{
    cout << "immstool distance|deviation|missing|purge|lint|help" << endl;
    return -1;
}

void do_help()
{
    cout << "immstool" << endl;
    cout << "  Available commands are:" << endl;
    cout << "       distance    - calculate distance from a given spectrum" << endl;
    cout << "       deviation   - calculate statistics on a list of spectrums" << endl;
    cout << "       missing     - list missing files" << endl;
    cout << "       purge       - remove from database if last played more than n days ago" << endl;
    cout << "       lint        - remove unneeded entries" << endl;
    cout << "       help        - show this help" << endl;
}

time_t ImmsTool::get_last(const string &path)
{
    Q q("SELECT last FROM 'Last' "
                "INNER JOIN Library ON Last.sid = Library.sid "
                "WHERE Library.path = ?;");
    q << path;
    if (!q.next())
        return 0;

    time_t last;
    q >> last;
    return last;
} 

void ImmsTool::do_purge(const string &path)
{
    Q q("DELETE FROM 'Library' WHERE path = ?;");
    q << path;
    q.execute();
}

void ImmsTool::do_lint()
{
    Q( "DELETE FROM Info "
            "WHERE sid NOT IN (SELECT sid FROM Library);").execute();

    Q("DELETE FROM Last "
            "WHERE sid NOT IN (SELECT sid FROM Library);").execute();

    Q("DELETE FROM Rating "
            "WHERE uid NOT IN (SELECT uid FROM Library);").execute();

    Q("DELETE FROM Acoustic "
            "WHERE uid NOT IN (SELECT uid FROM Library);").execute();

    Q("DELETE FROM Correlations "
            "WHERE origin NOT IN (SELECT sid FROM Library) "
            "OR destination NOT IN (SELECT sid FROM Library);").execute();

    Q("VACUUM Library;").execute();
}

void ImmsTool::do_distance()
{
    Q q("SELECT Library.path, Acoustic.spectrum, Library.sid "
            "FROM 'Library' INNER JOIN 'Acoustic' ON "
            "Library.uid = Acoustic.uid WHERE Acoustic.spectrum NOT NULL;");
    while(q.next())
    {
        string path, spectrum;
        int sid;
        q >> path >> spectrum >> sid;

        distance_callback(path, spectrum, sid);
    }
                
}

int ImmsTool::distance_callback(const string &path,
        const string &spectrum, int sid)
{
    cout << setw(4) << rms_string_distance(g_argv[2], spectrum)
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

    return 0;
}

void ImmsTool::do_missing()
{
    Q q("SELECT path FROM 'Library';");

    while (q.next())
    {
        string path;
        q >> path;
        if (access(path.c_str(), F_OK))
            cout << path << endl;
    }
}

void do_deviation_2(const string &filename)
{
    ifstream in(filename.c_str());

    int count = 0;
    double mean[SHORTSPECTRUM];
    memset(&mean, 0, sizeof(mean));
    double cur[SHORTSPECTRUM];
    while (1)
    {
        for (int i = 0; i < SHORTSPECTRUM; ++i)
            in >> cur[i];

        if (in.eof())
            break;

        ++count;
        for (int i = 0; i < SHORTSPECTRUM; ++i)
            mean[i] += cur[i];
    }

    // total to mean
    for (int i = 0; i < SHORTSPECTRUM; ++i)
        mean[i] /= count;

    cout << "Mean     : ";
    for (int i = 0; i < SHORTSPECTRUM; ++i)
    cout << endl;

    double deviations[SHORTSPECTRUM];
    memset(&deviations, 0, sizeof(deviations));

    in.close();
    ifstream in2(filename.c_str());

    while (1)
    {
        for (int i = 0; i < SHORTSPECTRUM; ++i)
            in2 >> cur[i];

        if (in2.eof())
            break;

        for (int i = 0; i < SHORTSPECTRUM; ++i)
            deviations[i] += pow(mean[i] - cur[i], 2);
    }

    for (int i = 0; i < SHORTSPECTRUM; ++i)
        deviations[i] = sqrt(deviations[i] / count);

    cout.precision(4);
    cout << "Results : " << endl;
    for (int i = 0; i < SHORTSPECTRUM; ++i)
    {
        double div = deviations[i] * 2.5;
        double min = std::max(mean[i] - div, 0.0);
        double max = mean[i] + div;
        double scale = 26 / (max - min);
        cout << setw(7) << min << " .. " << setw(5) << max;
        cout << "    [" << setw(5) << scale << " / " << setw(6)
            << (min < 1 ? 0 : min) << "]" << endl;
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
