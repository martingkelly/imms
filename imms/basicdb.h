#ifndef __BASICDB_H
#define __BASICDB_H

#include <string>
#include <utility>
#include <time.h>

#include "sqldb2.h"

using std::string;
using std::pair;

typedef pair<string, string> StringPair;
typedef pair<int, int> IntPair;

class BasicDb : protected SqlDb
{
public:
    BasicDb();
    virtual ~BasicDb();
    int identify(const string &path, time_t modtime);
    int identify(const string &path, time_t modtime,
            const string &checksum);

    int get_rating();
    time_t get_last();
    IntPair get_id();
    StringPair get_info();
    string get_spectrum();
    string get_bpm();

    void set_last(time_t last);
    void set_title(const string &_title);
    void set_artist(const string &_artist);
    void set_spectrum(const string &spectrum);
    void set_bpm(const string& bpm);
    void set_rating(int rating);
    void set_id(const IntPair &p);

    bool check_artist(string &artist);
    bool check_title(string &title);

    int avg_rating();

protected:
    void register_new_sid();

    void sql_set_pragma();
    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0);

    // state cache
    string bpm;
    int uid, sid;
    string artist, title;
};

#endif
