#ifndef __IMMSDB_H
#define __IMMSDB_H

#include <string>
#include <utility>
#include <time.h>

#include "sqldb.h"

using std::string;
using std::pair;

typedef pair<string, string> StringPair;
typedef pair<int, int> IntPair;

class ImmsDb : protected SqlDb
{
public:
    ImmsDb();
    ~ImmsDb();
    int identify(const string &path, time_t modtime);
    int identify(const string &path, time_t modtime, const string &checksum);

    int get_rating();
    int correlate(int from);
    time_t get_last();
    IntPair get_id();
    StringPair get_info();

    void set_last(time_t last);
    void set_title(const string &_title);
    void set_artist(const string &_artist);
    void set_rating(int rating);
    void set_id(const IntPair &p);

    void add_recent(int weight);

    bool check_artist(string &artist);
    bool check_title(string &title);

protected:
    void register_new_sid(int new_sid = -1);

    void expire_recent(const string& where_clause);
    void update_correlation(int from, int to, float weight);
    int expire_recent_callback_1(int argc, char **argv);
    int expire_recent_callback_2(int argc, char **argv);
    int update_secondaty_correlations(int argc, char **argv);
    void update_secondaty_correlations_2(int p, int q);

    void sql_create_tables();
    void sql_schema_upgrade();

    // shared within callbacks
    int from, from_weight, to, to_weight;
    float weight;

    // state cache
    int uid, sid;
    string artist, title;
};

#endif
