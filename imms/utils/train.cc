#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <list>
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
#include <normal.h>
#include <appname.h>

#include <analyzer/distance.h>
#include <analyzer/beatkeeper.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::cin;
using std::vector;
using std::list;
using std::ofstream;

const string AppName = IMMSTOOL_APP;

struct Sample {
    Sample(int CLASS) : CLASS(CLASS) {}
    int uid1, uid2, CLASS;
};

typedef vector<Sample> Samples;
typedef list< vector<float> > Features;

void collect_samples(Samples *samples,
        const string &table_name, int CLASS)
{
    try {
    Q q("SELECT * FROM " + table_name + ";");
    while (q.next()) {
        Sample s(CLASS);
        q >> s.uid1 >> s.uid2;
        samples->push_back(s);
    }
    } WARNIFFAILED();
}

float find_max(float a[BEATSSIZE])
{
    return *std::max_element(a, a + BEATSSIZE);
}

float find_min(float a[BEATSSIZE])
{
    return *std::min_element(a, a + BEATSSIZE);
}

void mm_sums(const MixtureModel &mm, float sums[3])
{
    for (int i = 0; i < 3; ++i)
        sums[i] = 0;
    for (int i = 0; i < NUMGAUSS; ++i)
    {
        const Gaussian &g = mm.gauss[i];
        for (int j = 0; j < NUMCEPSTR; ++j)
            sums[j / 5] += g.weight * g.means[j];
    }
}

void collect_features(Features *features, const Samples &samples) {
    MixtureModel mm1, mm2;
    float b1[BEATSSIZE], b2[BEATSSIZE];
    float sums1[3], sums2[3];

    AutoTransaction a;

    for (unsigned i = 0, size = samples.size(); i < size; ++i) {
        vector<float> f;
        const Sample &s = samples[i];
        Song s1("", s.uid1), s2("", s.uid2);

        s1.get_acoustic(&mm1, sizeof(MixtureModel), b1, sizeof(b1));
        s2.get_acoustic(&mm2, sizeof(MixtureModel), b2, sizeof(b2));

        f.push_back(EMD::raw_distance(mm1, mm2));
        f.push_back(EMD::raw_distance(b1, b2));

        mm_sums(mm1, sums1);
        mm_sums(mm2, sums2);

        for (int j = 0; j < 3; ++j)
        {
            f.push_back(sums1[j]);
            f.push_back(sums2[j]);
        }

        f.push_back(find_max(b1));
        f.push_back(find_max(b2));

        f.push_back(find_min(b1));
        f.push_back(find_min(b2));

        f.push_back(samples[i].CLASS);

        features->push_back(f);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    { 
        LOG(ERROR) << "Usage: ./train <limit> <datafile>" << endl;
        return -1;
    }   

    int limit = atoi(argv[1]);
    DEBUGVAL(limit);
    if (!limit)
        return -2;

    ImmsDb immsdb;

    try
    {
        QueryCacheDisabler qcd;
        Q("CREATE TEMP TABLE Inverse "
                "('sid' INTEGER UNIQUE NOT NULL, "
                "'uid' INTEGER NOT NULL);").execute();

        Q("INSERT INTO Inverse "
                "SELECT sid, uid FROM (SELECT sid, uid, max(R.rating) "
                "FROM Library NATURAL INNER JOIN Ratings R "
                "GROUP BY sid);").execute();

        string select_start = 
            "SELECT i1.uid, i2.uid FROM C.Correlations R "
            "INNER JOIN Inverse i1 on R.x = i1.sid "
            "INNER JOIN Inverse i2 on R.y = i2.sid "
            "ORDER BY ";
        string select_end = " LIMIT " + itos(limit) + ";";

        Q("CREATE TEMP TABLE PositiveUids AS "
                + select_start + "-weight" + select_end).execute();
        Q("CREATE TEMP TABLE NegativeUids AS "
                + select_start + "weight" + select_end).execute();
    } WARNIFFAILED();

    Samples samples;
    collect_samples(&samples, "PositiveUids", 1);
    collect_samples(&samples, "NegativeUids", 0);

    DEBUGVAL(samples.size());

    Features features;
    collect_features(&features, samples);

    DEBUGVAL(features.size());

    if (!features.size() || !features.front().size())
        return -3;

    ofstream f(argv[2]);
    f << features.size() << " " << features.front().size() << endl;

    for (Features::iterator i = features.begin(); i != features.end(); ++i)
    {
        const vector<float> &v = *i;
        f << v[0];
        for (unsigned j = 1, size = v.size(); j < size; ++j)
            f << " " << v[j];
        f << endl;
    }

    f.close();

    return 0;
}
