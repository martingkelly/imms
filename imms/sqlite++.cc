#include <sqlite3.h>
#include <sstream>

#include "sqlite++.h"

using std::ostringstream;

SQLExec execute;

// SQLDatabase

class SQLDatabase
{
public:
    static sqlite3 *db();
    static string error();
    
protected:
    friend class SQLDatabaseConnection;

    static void open(const string &filename);
    static void close();

    static sqlite3 *db_ptr;
};

sqlite3* db() { return SQLDatabase::db(); }

sqlite3 *SQLDatabase::db_ptr;

void SQLDatabase::open(const string &filename)
{
    if (db_ptr)
        close();
    if (sqlite3_open(filename.c_str(), &db_ptr))
        throw SQLStandardException();

    sqlite3_busy_timeout(db(), 5000);
}

void SQLDatabase::close()
{
    SQLQueryManager::self()->kill();
    if (db_ptr)
        sqlite3_close(db_ptr);
    db_ptr = 0;
}

string SQLDatabase::error()
{
    return sqlite3_errmsg(db_ptr);
}

sqlite3 *SQLDatabase::db()
{
    if (!db_ptr)
        throw SQLException("Error", "db_ptr not intialized");
    return db_ptr;
} 

SQLException::SQLException(const string &file, int line, const string &error)
{
    ostringstream sstr;
    sstr << file << ":" << line << ": " << error;
    msg = sstr.str().c_str();
}

// SQLDatabaseConnection

SQLDatabaseConnection::SQLDatabaseConnection(const string &filename)
{
    open(filename);
}

void SQLDatabaseConnection::open(const string &filename)
{
    SQLDatabase::open(filename);
}

SQLDatabaseConnection::~SQLDatabaseConnection()
{
    close();
}

void SQLDatabaseConnection::close()
{
    SQLDatabase::close();
}

string SQLDatabaseConnection::error()
{
    return SQLDatabase::error();
}

// AttachedDatabase

AttachedDatabase::AttachedDatabase(const string &filename, const string &alias)
{
    attach(filename, alias);
}

AttachedDatabase::~AttachedDatabase()
{
    detach();
}

void AttachedDatabase::attach(const string &filename, const string &alias)
{
    if (dbname != "")
        throw SQLException("Database already attached!");
    QueryCacheDisabler qcd;
    dbname = alias;
    Q("ATTACH \"" + filename + "\" AS " + dbname).execute();
}

void AttachedDatabase::detach()
{
    if (dbname == "")
        throw SQLException("No database attached!");
    QueryCacheDisabler qcd;
    string query = "DETACH " + dbname; 
    dbname = "";
    Q(query).execute();
}

// AutoTransaction

AutoTransaction::AutoTransaction() : commited(false)
{
    if (sqlite3_exec(SQLDatabase::db(), "BEGIN TRANSACTION;", 0, 0, 0))
        commited = true;
}

AutoTransaction::~AutoTransaction()
{
    if (!commited)
        sqlite3_exec(SQLDatabase::db(), "ROLLBACK TRANSACTION;", 0, 0, 0);
}

void AutoTransaction::commit()
{
    if (!commited)
        sqlite3_exec(SQLDatabase::db(), "COMMIT TRANSACTION;", 0, 0, 0);
    commited = true;
}

// SQLQueryManager

SQLQueryManager *SQLQueryManager::instance;

sqlite3_stmt *SQLQueryManager::get(const string &query)
{
    StmtMap::iterator i = statements.find(query);

    if (i != statements.end())
        return i->second;

#ifdef TRANS
    int tr = 1;
    if (cache)
        tr = sqlite3_exec(SQLDatabase::db(), "BEGIN TRANSACTION;", 0, 0, 0);
#endif

    sqlite3_stmt *statement = 0;
    int qr = sqlite3_prepare(SQLDatabase::db(), query.c_str(),
            -1, &statement, 0);

    SQLException except = SQLStandardException();

#ifdef TRANS
    if (tr == 0) 
        if (sqlite3_exec(SQLDatabase::db(), "COMMIT TRANSACTION;", 0, 0, 0))
            throw SQLStandardException();
#endif

    if (qr)
    {
        if (block)
            return 0;
        throw except;
    }

    if (cache)
        statements[query] = statement;
    return statement;
}

SQLQueryManager *SQLQueryManager::self()
{
    if (!instance)
        instance = new SQLQueryManager();
    return instance;
}

void SQLQueryManager::kill()
{
    delete instance;
    instance = 0;
}

SQLQueryManager::~SQLQueryManager()
{
    for (StmtMap::iterator i = statements.begin(); i != statements.end(); ++i)
        sqlite3_finalize(i->second);
}

// SQLQuery

SQLQuery::SQLQuery(const string &query) : curbind(0), stmt(0)
{
    stmt = SQLQueryManager::self()->get(query);
}

bool SQLQuery::next()
{
    if (!stmt)
        return false;

    curbind = 0;
    int r = sqlite3_step(stmt);
    if (r == SQLITE_ROW)
        return true;
    reset();
    if (r != SQLITE_DONE && r != SQLITE_CONSTRAINT)
        throw SQLStandardException();
    return false;
}

void SQLQuery::reset()
{
    curbind = 0;
    if (stmt)
        sqlite3_reset(stmt);
}

SQLQuery &SQLQuery::operator<<(int i)
{
    if (stmt)
        if (sqlite3_bind_int(stmt, ++curbind, i))
            throw SQLStandardException();
    return *this;
}

SQLQuery &SQLQuery::operator<<(double i)
{
    if (stmt)
        if (sqlite3_bind_double(stmt, ++curbind, i))
            throw SQLStandardException();
    return *this;
}

SQLQuery &SQLQuery::operator<<(const string &s)
{
    if (stmt)
        if (sqlite3_bind_text(stmt, ++curbind, s.c_str(), -1, SQLITE_TRANSIENT))
            throw SQLStandardException();
    return *this;
}

SQLQuery &SQLQuery::operator<<(const SQLExec &execute)
{
    this->execute();
    return *this;
}

SQLQuery &SQLQuery::operator>>(int &i)
{
    if (stmt)
        i = sqlite3_column_int(stmt, curbind++);
    return *this;
}

SQLQuery &SQLQuery::operator>>(string &s)
{
    s = "";
    if (stmt)
    {
        char *c = (char *)sqlite3_column_text(stmt, curbind++);
        if (c)
            s = c;
    }
    return *this;
}

SQLQuery &SQLQuery::operator>>(double &i)
{
    if (stmt)
        i = sqlite3_column_double(stmt, curbind++);
    return *this;
}

SQLQuery &SQLQuery::operator>>(float &i)
{
    double j = 0;
    *this >> j;
    i = j;
    return *this;
}
