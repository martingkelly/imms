#ifndef __BASICDB_H
#define __BASICDB_H

#include <string>

#include "sqldb2.h"

using std::string;

class BasicDb : protected SqlDb
{
public:
    BasicDb();
    virtual ~BasicDb();

    bool check_artist(string &artist);
    bool check_title(const string &artist, string &title);

    int avg_playcounter();

    void update_all_ratings();
    void fake_encounter(int uid, int delta, time_t time);

protected:
    void sql_set_pragma();
    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0);

private:
    void mass_rating_upgrade();
    void sql_create_v8_tables();
};

#endif
