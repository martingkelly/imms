#ifndef __SQLITE___H
#define __SQLITE___H

#include "immsconf.h"
#include <string>
#include <iostream>
#include <map>
#include <stdint.h>

using std::string;
using std::cerr;
using std::endl;

class SQLExec {};
extern SQLExec execute;

class SQLDatabaseConnection
{
public:
    SQLDatabaseConnection(const string &filename);
    SQLDatabaseConnection() {};
    ~SQLDatabaseConnection();

    static uint64_t last_rowid();

    void open(const string &filename);
    void close();
    static string error();
};

class AttachedDatabase
{
public:
    AttachedDatabase(const string &filename, const string &alias);
    AttachedDatabase() : dbname("") {};
    ~AttachedDatabase();

    void attach(const string &filename, const string &alias);
    void detach();
private:
    string dbname;
};

class AutoTransaction
{
public:
    AutoTransaction(bool exclusive = false);
    ~AutoTransaction();
    void commit();
private:
    bool commited;
};

class SQLException : public std::exception
{
public:
    SQLException(const string &source = "Error",
            const string &error = SQLDatabaseConnection::error())
        : msg(source + ": " + error) {};
    SQLException(const string &file, int line, const string &error);
    const string &what() { return msg; }
    ~SQLException() throw () {};
private:
    string msg;
};

typedef struct sqlite3_stmt sqlite3_stmt;

class SQLQueryManager
{
public:
    SQLQueryManager() : block_errors(false) {}
    sqlite3_stmt *get(const string &query);
    ~SQLQueryManager();

    static SQLQueryManager *self();
    static void kill();
private:
    typedef std::map<string, sqlite3_stmt *> StmtMap;
    StmtMap statements;

    friend class RuntimeErrorBlocker;
    bool block_errors;
    static SQLQueryManager *instance;
};

class RuntimeErrorBlocker
{
public:
    RuntimeErrorBlocker() : active(!SQLQueryManager::self()->block_errors)
        { SQLQueryManager::self()->block_errors = true; }
    ~RuntimeErrorBlocker()
        { if (active) SQLQueryManager::self()->block_errors = false; }
private:
    bool active;
};

class SQLQuery
{
public:
    SQLQuery(const string &query);
    ~SQLQuery();

    void reset();
    bool next();
    bool is_null();
    bool not_null() { return !is_null(); }
    void execute() { while (next()); }

    SQLQuery &operator()(int bindto) { curbind = bindto; return *this; }
    SQLQuery &operator<<(int i);
    SQLQuery &operator<<(double i);
    SQLQuery &operator<<(float i) { return *this << double(i); }
    SQLQuery &operator<<(long i);
    SQLQuery &operator<<(const string &s);
    SQLQuery &operator<<(const SQLExec &execute);

    SQLQuery &bind(const void *data, size_t n);

    SQLQuery &operator>>(int &i);
    SQLQuery &operator>>(long &i);
    SQLQuery &operator>>(double &i);
    SQLQuery &operator>>(float &i);
    SQLQuery &operator>>(string &s);

    SQLQuery &load(void *data, size_t &n);
    SQLQuery &load(void *data, unsigned long long n) {
        size_t real_size = n;
        return load(data, real_size);
    };

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

#define IGNOREFAILURE()                                                     \
    catch (SQLException &e) {                                               \
    } do {} while (0)

#define SQLStandardException()  \
    SQLException(__FILE__, __LINE__, SQLDatabaseConnection::error())

#endif
