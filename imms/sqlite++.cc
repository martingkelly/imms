#include <sqlite3.h>

#include "sqlite++.h"
#include "strmanip.h"

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
    : msg(file + ":" + itos(line) + ": " + error) {};

// SQLDatabaseConnection

SQLDatabaseConnection::SQLDatabaseConnection(const string &filename)
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

// AutoTransaction

AutoTransaction::AutoTransaction() : commited(false)
{
    try {
        SQLQuery("BEGIN TRANSACTION;").execute();
    }
    catch (SQLException &q)
    {
        commited = true;
    }
}

AutoTransaction::~AutoTransaction()
{
    if (!commited)
        SQLQuery("ROLLBACK TRANSACTION;").execute();
}

void AutoTransaction::commit()
{
    if (!commited)
        SQLQuery("COMMIT TRANSACTION;").execute();
    commited = true;
}

// SQLQueryManager

SQLQueryManager *SQLQueryManager::instance;

sqlite3_stmt *SQLQueryManager::get(const string &query)
{
    StmtMap::iterator i = statements.find(query);

    if (i != statements.end())
        return i->second;

    sqlite3_stmt *statement = 0;
    if (sqlite3_prepare(SQLDatabase::db(), query.c_str(), -1, &statement, 0))
    {
        if (block)
            return 0;
        else
            throw SQLStandardException();
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
        if (sqlite3_bind_text(stmt, ++curbind, s.c_str(), -1, SQLITE_STATIC))
            throw SQLStandardException();
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
    if (stmt)
        s = (char *)sqlite3_column_text(stmt, curbind++);
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
