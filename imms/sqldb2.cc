#include <iostream>
#include <sstream>
#include <sqlite3.h>

#include "strmanip.h"
#include "sqldb2.h"

using std::endl;
using std::cerr;

// Fuzzy compare function using levenshtein string distance
static void fuzzy_like(sqlite3_context *context, int argc, sqlite3_value** val)
{
    if (argc != 2)
        throw SQLException("fuzzy_like", "argc != 2");
    sqlite3_result_int(context, string_like(
                (char*)sqlite3_value_text(val[0]),
                (char*)sqlite3_value_text(val[1]), 4));
}

extern sqlite3 *db();

SqlDb::SqlDb(const string &dbname) : dbcon(dbname)
{
    sqlite3_create_function(db(), "similar", 2, 1, 0, fuzzy_like, 0, 0);
}

SqlDb::~SqlDb()
{
    close_database();
}

void SqlDb::close_database()
{
    dbcon.close();
}

int SqlDb::changes()
{
    return db() ? sqlite3_changes(db()) : 0;
}
