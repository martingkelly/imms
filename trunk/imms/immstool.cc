#include <string>
#include <iostream>
#include <iomanip>
#include <list>
#include <utility>

#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "imms.h"
#include "sqldb.h"
#include "strmanip.h"

using std::string;
using std::cout;
using std::endl;
using std::list;
using std::cin;
using std::setw;
using std::pair;

int g_argc;
char **g_argv;

extern pair<float, float> spectrum_analyze(const string &spectstr);
extern string spectrum_normalize(const string &spectr);
extern int spectrum_distance(const string &s1, const string &s2);

class ImmsTool : public SqlDb
{
public:
    ImmsTool()
        : SqlDb(string(getenv("HOME")).append("/.imms/imms.db")) { }

    void do_distance();
    int distance_callback(int argc, char *argv[]);
    void do_missing();
    int missing_callback(int argc, char *argv[]);
    time_t get_last(const string &path);
    void do_purge(const string &path);
    void do_lint();
};

int usage();
void do_help();
void do_deviation();

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
    cout << "immstool distance|deviation|missing|purge|help" << endl;
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
    cout << "       help        - show this help" << endl;
}

time_t ImmsTool::get_last(const string &path)
{
    select_query(
            "SELECT last FROM 'Last' "
                "INNER JOIN Library ON Last.sid = Library.sid "
                "WHERE Library.path = '" + escape_string(path) + "';");

    return nrow && resultp[1] ? atol(resultp[1]) : 0;
} 

void ImmsTool::do_purge(const string &path)
{
    run_query("DELETE FROM 'Library' WHERE path = '"
            + escape_string(path) + "'");
}

void ImmsTool::do_lint()
{
    run_query(
            "DELETE FROM Info "
            "WHERE sid NOT IN (SELECT sid FROM Library);");

    run_query(
            "DELETE FROM Last "
            "WHERE sid NOT IN (SELECT sid FROM Library);");

    run_query(
            "DELETE FROM Rating "
            "WHERE uid NOT IN (SELECT uid FROM Library);");

    run_query(
            "DELETE FROM Correlations "
            "WHERE origin NOT IN (SELECT sid FROM Library) "
            "OR destination NOT IN (SELECT sid FROM Library);");
}

void ImmsTool::do_distance()
{
    select_query("SELECT path, spectrum, sid FROM 'Library' "
            "WHERE spectrum NOT NULL;",
            (SqlCallback)&ImmsTool::distance_callback);
}

int ImmsTool::distance_callback(int argc, char *argv[])
{
    assert(argc == 3);
    cout << setw(4) << spectrum_distance(
            spectrum_normalize(g_argv[2]),
            spectrum_normalize(argv[1]))
        << "  " << argv[1] << "  ";

    select_query("SELECT artist, title FROM Info "
            "WHERE sid = '" + string(argv[2]) + "';");

    if (nrow && resultp[1])
    {
        cout << setw(25) << resultp[2];
        cout << setw(25) << resultp[3];
    }
    else
        cout << setw(50) << path_get_filename(argv[0]);
    cout << endl;

    return 0;
}

void ImmsTool::do_missing()
{
    select_query("SELECT path FROM 'Library';",
            (SqlCallback)&ImmsTool::missing_callback);
}

int ImmsTool::missing_callback(int argc, char *argv[])
{
    assert(argc == 1);

    if (access(argv[0], F_OK))
        cout << argv[0] << endl;

    return 0;
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
        spectrum = spectrum_normalize(spectrum);
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
