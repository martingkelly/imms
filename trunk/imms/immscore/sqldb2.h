#ifndef __SQLDB_H
#define __SQLDB_H

#include <string>
#include "sqlite++.h"

using std::string;

class SqlDb
{
public:
    SqlDb();
    ~SqlDb();

protected:
    void close_database();
    int changes();

private:
    AttachedDatabase *correlations;
    SQLDatabaseConnection dbcon;
};

#endif
