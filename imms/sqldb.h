#ifndef __SQLDB_H
#define __SQLDB_H

#include <string>

using std::string;

struct ImmsCallbackBase
{
    virtual int call(int argc, char **argv) = 0;
};

template <typename C>
struct ImmsCallback : public ImmsCallbackBase
{
    typedef int (C::*F)(int argc, char **argv);
    ImmsCallback(C *i, F c) : instance(i), callback(c) {}
    C *instance;
    F callback;
    virtual int call(int argc, char **argv)
         { return (instance->*callback)(argc, argv); } 
};

typedef int (*SQLiteCallback)(void*,int,char**,char**);
struct sqlite;

class SqlDb
{
public:
    SqlDb(const string &dbname);
    ~SqlDb();

protected:
    bool run_query(const string &query);
    void select_query(const string &query);
    void select_query(const string &query, ImmsCallbackBase *callback,
            int tmpcolumns = 0);
    void close_database();
    int changes();

    int nrow, ncol;
    char **resultp;

private:
    void handle_error(const string &query);
    char *errmsg;
    struct sqlite *db;
    int tmptables;
};

#endif
