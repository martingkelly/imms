#ifndef __SQLITE___H
#define __SQLITE___H

#include <string>
#include <map>

using std::string;
using std::map;

class SQLDatabaseConnection
{
public:
    SQLDatabaseConnection(const string &filename);
    ~SQLDatabaseConnection();

    void close();
    static string error();
};

class AutoTransaction
{
public:
    AutoTransaction();
    ~AutoTransaction();
    void commit();
private:
    bool active, commited;
};

class SQLException
{
public:
    SQLException(const string &source = "Error",
            const string &error = SQLDatabaseConnection::error())
        : msg(source + ": " + error) {};
    SQLException(const string &file, int line, const string &error);
    const string &what() { return msg; }
private:
    string msg;
};

typedef struct sqlite3_stmt sqlite3_stmt;

class SQLQueryManager
{
public:
    SQLQueryManager() : cache(true) {}
    sqlite3_stmt *get(const string &query);
    ~SQLQueryManager();

    static SQLQueryManager *self();
    static void kill();
private:
    typedef std::map<string, sqlite3_stmt *> StmtMap;
    StmtMap statements;

    friend class QueryCacheDisabler;
    friend class RuntimeErrorBlocker;
    bool cache, block;
    static SQLQueryManager *instance;
};

class QueryCacheDisabler
{
public:
    QueryCacheDisabler() : active(SQLQueryManager::self()->cache)
        { SQLQueryManager::self()->cache = false; }
    ~QueryCacheDisabler()
        { if (active) SQLQueryManager::self()->cache = true; }
private:
    bool active;
};

class RuntimeErrorBlocker
{
public:
    RuntimeErrorBlocker() : active(!SQLQueryManager::self()->block)
        { SQLQueryManager::self()->block = true; }
    ~RuntimeErrorBlocker()
        { if (active) SQLQueryManager::self()->block = false; }
private:
    bool active;
};

class SQLQuery
{
public:
    SQLQuery(const string &query);
    ~SQLQuery() { reset(); }

    void reset();
    bool next();
    void execute() { while (next()); }

    SQLQuery &operator()(int bindto) { curbind = bindto; return *this; }
    SQLQuery &operator<<(int i);
    SQLQuery &operator<<(double i);
    SQLQuery &operator<<(float i) { return *this << double(i); }
    SQLQuery &operator<<(time_t i) { return *this << (int)i; }
    SQLQuery &operator<<(const string &s);

    SQLQuery &operator>>(int &i);
    SQLQuery &operator>>(double &i);
    SQLQuery &operator>>(float &i);
    SQLQuery &operator>>(time_t &i) { return *this >> ((int&)i); }
    SQLQuery &operator>>(string &s);

private:
    int curbind;

    sqlite3_stmt *stmt;
};

typedef SQLQuery Q;

#define WARNIFFAILED()                                                      \
    catch (SQLException &e) {                                               \
        cerr << string(80, '*') << endl;                                    \
        cerr << __FILE__ << ":" << __func__ << ": " << e.what() << endl;    \
        cerr << string(80, '*') << endl;                                    \
    } do {} while (0)

#define SQLStandardException()  \
    SQLException(__FILE__, __LINE__, SQLDatabaseConnection::error())

#endif
