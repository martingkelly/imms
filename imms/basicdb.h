#ifndef __BASICDB_H
#define __BASICDB_H

#include <string>

#include "sqldb2.h"
#include "song.h"

using std::string;

class BasicDb : protected SqlDb, public Song
{
public:
    BasicDb();
    virtual ~BasicDb();

    bool check_artist(string &artist);
    bool check_title(const string &artist, string &title);

    int avg_rating(const string &title, const string &artist);

    bool identify(const string &path)
        { *((Song*)this) = Song::identify(path); return uid != -1; }

protected:
    void sql_set_pragma();
    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0);
};

#endif
