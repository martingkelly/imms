#include <iostream>
#include <sstream>
#include <sqlite.h>

#include "strmanip.h"
#include "utils.h"
#include "sqldb.h"

using std::endl;
using std::cerr;
using std::ostringstream;

static int callback_helper(void *cbdata, int argc, char **argv, char **)
{
    ImmsCallbackBase *callback = (ImmsCallbackBase*)cbdata;
    return callback->call(argc, argv);
}

// Fuzzy compare function using levenshtein string distance
static void fuzzy_like(sqlite_func *context, int arg, const char **argv)
{
    if (!argv[0] || !argv[1])
        return;
    sqlite_set_result_int(context, string_like(argv[0], argv[1], 4));
}

static void sample(sqlite_func *context, int arg, const char **argv)
{
    if (!argv[0] || !argv[1])
        return;
    int want = atoi(argv[0]);
    int total = atoi(argv[1]);
    sqlite_set_result_int(context, imms_random(total) <= want);
}

SqlDb::SqlDb(const string &dbname) : nrow(0), ncol(0), resultp(0), errmsg(0)
{
    tmptables = 0;
    db = sqlite_open(dbname.c_str(), 600, &errmsg);
    if (!db)
    {
        cerr << "Failed to open database '" << dbname << "'" << endl;
        return;
    }
    sqlite_busy_timeout(db, 1000);
    sqlite_create_function(db, "similar", 2, fuzzy_like, 0);
    sqlite_create_function(db, "sample", 2, sample, 0);
}

SqlDb::~SqlDb()
{
    sqlite_free_table(resultp);
    close_database();
}

int SqlDb::changes()
{
    return db ? sqlite_changes(db) : 0;
}

void SqlDb::close_database()
{
    if (db)
        sqlite_close(db);
    db = NULL;
    nrow = ncol = 0;
}

bool SqlDb::run_query(const string &query)
{
    if (db)
    {
        sqlite_exec(db, query.c_str(), 0, 0, &errmsg);
        bool res = !errmsg;
        handle_error(query);
        return res;
    }
    cerr << "Database not open!" << endl;
    return false;
}

void SqlDb::select_query(const string &query)
{
    if (!db)
    {
        cerr << "Database not open!" << endl;
        return;
    }

    sqlite_free_table(resultp);

    sqlite_get_table(db, query.c_str(), &resultp, &nrow, &ncol, &errmsg);
    handle_error(query);
}

void SqlDb::select_query(const string &query, ImmsCallbackBase *callback,
        int tmpcolumns)
{
    if (!db)
    {
        cerr << "Database not open!" << endl;
        return;
    }

    if (tmpcolumns)
    {
        string tablename = "tmp_" + itos(++tmptables);
        string createquery =
            "CREATE TEMP TABLE " + tablename + " AS " + query;
        string selectquery = "SELECT * FROM " + tablename + ";";
        string dropquery = "DROP TABLE " + tablename + ";";

        run_query(createquery);
        handle_error(createquery);

        select_query(selectquery, callback);

        run_query(dropquery);
        handle_error(dropquery);

        --tmptables;
    }
    else
    {
        sqlite_exec(db, query.c_str(), &callback_helper, callback, &errmsg);
        handle_error(query);
    }
}

void SqlDb::handle_error(const string &query)
{
    if (errmsg
            && !strstr(errmsg, "already exists")
            && !strstr(errmsg, "uniqueness constraint failed")
            && !strstr(errmsg, "is not unique")
            && !strstr(errmsg, "requested query abort")
            && !strstr(errmsg, "no such table"))
    {
        nrow = ncol = 0;
        cerr << errmsg << endl;
        cerr << "while executing: " << query << endl;
    }
    free(errmsg);
    errmsg = 0;
}
