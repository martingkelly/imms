#ifndef __IMMSDB_H
#define __IMMSDB_H

#include <string>
#include <utility>
#include <time.h>
#include <sys/time.h>

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
    int identify(const string &path, time_t modtime,
            const string &checksum);

    int get_rating();
    float correlate(int from);
    time_t get_last();
    IntPair get_id();
    StringPair get_info();
    string get_spectrum();
    int get_bpm();

    void set_last(time_t last);
    void set_title(const string &_title);
    void set_artist(const string &_artist);
    void set_spectrum(const string &spectrum);
    void set_bpm(int bpm);
    void set_rating(int rating);
    void set_id(const IntPair &p);

    void add_recent(int weight);

    bool check_artist(string &artist);
    bool check_title(string &title);

    int avg_rating();

    void clear_recent() { expire_recent(""); }

protected:
    void register_new_sid(int new_sid = -1);

    void expire_recent(const string& where_clause);
    void update_correlation(int from, int to, float weight);
    int expire_recent_callback_1(int argc, char **argv);
    int expire_recent_callback_2(int argc, char **argv);
    int update_secondaty_correlations(int argc, char **argv);

    void sql_set_pragma();
    void sql_create_tables();
    void sql_schema_upgrade();

    // shared within callbacks
    bool abort_requested;
    int from, from_weight, to, to_weight;
    float weight;

    // state cache
    int bpm;
    int uid, sid;
    string artist, title;

private:
    struct timeval start;
};

#endif
