#ifndef __SQLDB_H
#define __SQLDB_H

#include <string>

using std::string;
struct sqlite;

class SqlDb;
typedef int (SqlDb::*SqlCallback)(int,char**);

class SqlDb
{
public:
    SqlDb(const string &dbname);
    ~SqlDb();

protected:
    void run_query(const string &query);
    void select_query(const string &query);
    void select_query(const string &query, SqlCallback callback,
            int tmpcolumns = 0);
    void close_database();
    int changes();

    int nrow, ncol;
    char **resultp;

private:
    bool handle_error(const string &query);
    char *errmsg;
    struct sqlite *db;
    int tmptables;
};

#endif
