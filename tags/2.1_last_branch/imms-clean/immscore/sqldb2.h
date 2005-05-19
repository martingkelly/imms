#ifndef __SQLDB_H
#define __SQLDB_H

#include <string>
#include <memory>
#include "sqlite++.h"

using std::string;
using std::auto_ptr;

class SqlDb
{
public:
    SqlDb();
    ~SqlDb();

protected:
    void close_database();
    int changes();

private:
    auto_ptr<AttachedDatabase> correlations, acoustic;
    SQLDatabaseConnection dbcon;
};

#endif
