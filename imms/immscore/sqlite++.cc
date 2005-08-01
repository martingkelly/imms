#include <sqlite3.h>
#include <sstream>
#include <time.h>

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

    sqlite3_busy_timeout(db(), 30000);
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

uint64_t SQLDatabaseConnection::last_rowid()
{
    return sqlite3_last_insert_rowid(SQLDatabase::db_ptr);
}

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

AutoTransaction::AutoTransaction(bool exclusive) : commited(true)
{
    if (!sqlite3_get_autocommit(SQLDatabase::db()))
        return;

    int r;
    string exclusive_term = exclusive ? "EXCLUSIVE " : "";
    string query = "BEGIN " + exclusive_term + " TRANSACTION;";
    do { r = sqlite3_exec(SQLDatabase::db(), query.c_str(), 0, 0, 0); }
    while (r == SQLITE_BUSY);

    if (r)
        throw SQLStandardException();

    commited = false;
}

AutoTransaction::~AutoTransaction()
{
    if (!commited)
        sqlite3_exec(SQLDatabase::db(), "ROLLBACK TRANSACTION;", 0, 0, 0);
}

void AutoTransaction::commit()
{
    if (commited)
        return;

    commited = true;

    int r;
    while (1)
    {
        r = sqlite3_exec(SQLDatabase::db(),
                "COMMIT TRANSACTION;", 0, 0, 0);
        if (!r)
            break;
        if (r != SQLITE_BUSY)
            throw SQLStandardException();
#ifdef DEBUG
        cerr << "database busy at commit time - sleeping" << endl;
#endif
        struct timespec t = { 0, 250000000 };
        nanosleep(&t, 0);
    }
}

// SQLQueryManager

SQLQueryManager *SQLQueryManager::instance;

sqlite3_stmt *SQLQueryManager::get(const string &query, bool &cached)
{
    StmtMap::iterator i = statements.find(query);

    if (i != statements.end())
    {
        if (!sqlite3_expired(i->second))
            return i->second;
        sqlite3_finalize(i->second);
        statements.erase(query);
    }

    sqlite3_stmt *statement = 0;
    int qr = sqlite3_prepare(SQLDatabase::db(), query.c_str(),
            -1, &statement, 0);

    SQLException except = SQLStandardException();

    if (qr)
    {
        if (block)
            return 0;
        throw except;
    }

    cached = cache;
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

SQLQuery::SQLQuery(const string &query) : curbind(0), cached(true), stmt(0)
{
    stmt = SQLQueryManager::self()->get(query, cached);
}

SQLQuery::~SQLQuery()
{
    reset();

    if (stmt && !cached)
        sqlite3_finalize(stmt);
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

SQLQuery &SQLQuery::operator<<(long i)
{
    if (stmt)
    {
        if (sizeof(long) == 8)
        {
            if (sqlite3_bind_int64(stmt, ++curbind, i))
                throw SQLStandardException();
        }
        else
        {
            if (sqlite3_bind_int(stmt, ++curbind, i))
                throw SQLStandardException();
        }
    }
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

SQLQuery &SQLQuery::bind(const void *data, size_t n)
{
    if (stmt)
        if (sqlite3_bind_blob(stmt, ++curbind, data, n, SQLITE_TRANSIENT))
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

SQLQuery &SQLQuery::operator>>(long &i)
{    
    if (stmt)
    {
        if (sizeof(long) == 8)
            i = sqlite3_column_int64(stmt, curbind++);
        else 
            i = sqlite3_column_int(stmt, curbind++);
    }
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

SQLQuery &SQLQuery::load(void *data, size_t &n)
{
    if (!stmt)
        return *this;

    size_t blobsize = sqlite3_column_bytes(stmt, curbind);
    if (blobsize > n)
        throw SQLException("Error",
                "insufficient memory provided for load BLOB");
    n = blobsize;
    const void *blobdata = sqlite3_column_blob(stmt, curbind++);
    memcpy(data, blobdata, blobsize);
    return *this;
}

SQLQuery &SQLQuery::operator>>(float &i)
{
    double j = 0;
    *this >> j;
    i = j;
    return *this;
}