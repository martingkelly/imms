#include <string>
#include <iostream>
#include <iomanip>
#include <list>

#include <assert.h>
#include <stdlib.h>

#include "imms.h"
#include "sqldb.h"
#include "strmanip.h"

using std::string;
using std::cout;
using std::endl;
using std::list;
using std::cin;
using std::setw;

int usage()
{
    cout << "immstool distance|deviation|missing|help" << endl;
    return -1;
}

#include <utility>
using std::pair;
extern pair<float, float> spectrum_analyze(const string &spectstr);
extern string spectrum_normalize(const string &spectr);
extern int spectrum_distance(const string &s1, const string &s2);

class DistanceSqlDb : public SqlDb
{
public:
    DistanceSqlDb(const string &_reference)
        : SqlDb(string(getenv("HOME")).append("/.imms/imms.db"))
    { reference = _reference; }

    void do_stuff()
    {
        select_query("SELECT path,spectrum,sid FROM 'Library' "
                "WHERE spectrum NOT NULL;",
                (SqlCallback)&DistanceSqlDb::spectrum_callback);
    }

    int spectrum_callback(int argc, char *argv[])
    {
        assert(argc == 3);
        cout << setw(4) << spectrum_distance(
                spectrum_normalize(reference),
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

private:
    string reference;
};

class MissingSqlDb : public SqlDb
{
public:
    MissingSqlDb()
        : SqlDb(string(getenv("HOME")).append("/.imms/imms.db")) {}

    void do_stuff()
    {
        select_query("SELECT path FROM 'Library';",
                (SqlCallback)&MissingSqlDb::missing_callback);
    }

    int missing_callback(int argc, char *argv[])
    {
        assert(argc == 1);

        if (access(argv[0], F_OK))
                cout << argv[0] << endl;

        return 0;
    }
};

int main(int argc, char *argv[])
{
    if (argc < 2)
        return usage();

    if (!strcmp(argv[1], "distance"))
    {
        if (argc < 3)
        {
            cout << "immstool distance <reference spectrum>" << endl;
            return -1;
        }

        DistanceSqlDb mydb(argv[2]);
        mydb.do_stuff();
    }
    else if (!strcmp(argv[1], "deviation"))
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

        cout << "Mean	  : ";
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
    else if (!strcmp(argv[1], "missing"))
    {
        MissingSqlDb missing;
        missing.do_stuff();
    }
    else if (!strcmp(argv[1], "purge"))
    {
        string path;
        while (cin >> path)
        {

        }
    }
    else if (!strcmp(argv[1], "help"))
    {
        cout << "immstool" << endl;
        cout << "  Available commands are:" << endl;
        cout << "		distance	- calculate distance from a given spectrum" << endl;
        cout << "		deviation	- calculate statistics on a list of spectrums" << endl;
        cout << "		help		- show this help" << endl;
        return -1;
    }
    else
        return usage();
}
