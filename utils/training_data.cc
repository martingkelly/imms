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
#include <appname.h>

#include <model/distance.h>
#include <model/model.h>
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
    StatCollector<float> stats;
    float weight;
    try {
    Q q("SELECT * FROM " + table_name + ";");
    while (q.next()) {
        Sample s(CLASS);
        q >> s.uid1 >> s.uid2 >> weight;
        samples->push_back(s);
        stats.process(weight);
    }
    } WARNIFFAILED();
    stats.finish();
}

void collect_features(Features *features, const Samples &samples) {
    AutoTransaction a;

    MixtureModel mm1, mm2;
    float b1[BEATSSIZE], b2[BEATSSIZE];

    for (unsigned i = 0, size = samples.size(); i < size; ++i) {
        const Sample &s = samples[i];
        Song s1("", s.uid1), s2("", s.uid2);

        if (!s1.get_acoustic(&mm1, b1))
            continue;
        if (!s2.get_acoustic(&mm2, b2))
            continue;

        vector<float> f1;
        SimilarityModel::extract_features(mm1, b1, mm2, b2, &f1);
        f1.push_back(samples[i].CLASS);
        features->push_back(f1);

        vector<float> f2;
        SimilarityModel::extract_features(mm2, b2, mm1, b1, &f2);
        f2.push_back(samples[i].CLASS);
        features->push_back(f2);
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
                "'uid' INTEGER NOT NULL, "
                "'rating' INTEGER NOT NULL);").execute();

        Q("INSERT INTO Inverse "
                "SELECT sid, uid, max(R.rating) "
                "FROM Library NATURAL INNER JOIN Ratings R "
                "GROUP BY sid;").execute();

        string select_start = 
            "SELECT i1.uid, i2.uid, weight FROM C.Correlations R "
            "INNER JOIN Inverse i1 on R.x = i1.sid "
            "INNER JOIN Inverse i2 on R.y = i2.sid ";
        string select_end = " LIMIT " + itos(limit) + ";";

        string positive_query = "CREATE TEMP TABLE PositiveUids AS "
            + select_start;

        string negative_query = "CREATE TEMP TABLE NegativeUids AS "
            + select_start;

        if (true) {
            positive_query += "WHERE i1.rating < 95 AND i2.rating < 95 ";
            negative_query += "WHERE i1.rating > 60 AND i2.rating > 60 ";
        }

        Q(positive_query + "ORDER BY -weight" + select_end).execute();
        Q(negative_query + "ORDER BY weight" + select_end).execute();
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
