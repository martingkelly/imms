#ifndef __SQLDB_H
#define __SQLDB_H

#include <string>
#include "sqlite++.h"

using std::string;

class SqlDb
{
public:
    SqlDb(const string &dbname);
    ~SqlDb();

protected:
    void close_database();
    int changes();

private:
    SQLDatabaseConnection dbcon;
};

#endif
