#include <iostream>
#include <sstream>
#include <sqlite.h>

#include "sqldb.h"
#include "strmanip.h"

using std::endl;
using std::cerr;
using std::ostringstream;

struct CallbackData
{
    SqlDb *instance;
    SqlCallback callback;
    void *userdata;
};

typedef int (*SQLiteCallback)(void*,int,char**,char**);

static int callback_helper(void *cbdata, int argc, char **argv, char **)
{
    CallbackData *data = reinterpret_cast<CallbackData*>(cbdata);
    return (data->instance->*(data->callback))(argc, argv);
}

// Fuzzy compare function using levenshtein string distance
static void fuzzy_like(sqlite_func *context, int arg, const char **argv)
{
  if (argv[0] == 0 || argv[1] ==0)
      return;
  sqlite_set_result_int(context, string_like(argv[0], argv[1], 4));
}

SqlDb::SqlDb(const string &dbname) : nrow(0), ncol(0), resultp(0), errmsg(0)
{
    tmptables = 0;
    db = sqlite_open(dbname.c_str(), 600, &errmsg);
    if (!db)
        cerr << "Failed to open database '" << dbname << "'" << endl;
    else
        sqlite_create_function(db, "like", 2, fuzzy_like, 0);
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
}

void SqlDb::run_query(const string &query)
{
    if (db)
    {
        sqlite_exec(db, query.c_str(), 0, 0, &errmsg);
        handle_error(query);
    }
    else
    {
        cerr << "Database not open!" << endl;
    }
}

void SqlDb::select_query(const string &query)
{
    if (db)
    {
        sqlite_free_table(resultp);

        sqlite_get_table(db, query.c_str(), &resultp, &nrow, &ncol, &errmsg);
        handle_error(query);
    }
    else
    {
        cerr << "Database not open!" << endl;
    }
}

void SqlDb::select_query(const string &query, SqlCallback callback,
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
        CallbackData data;
        data.instance = this;
        data.callback = callback;

        sqlite_exec(db, query.c_str(), &callback_helper, &data, &errmsg);
        handle_error(query);
    }
}

bool SqlDb::handle_error(const string &query)
{
    int result = false;
    if (errmsg
            && !strstr(errmsg, "already exists")
            && !strstr(errmsg, "uniqueness constraint failed")
            && !strstr(errmsg, "is not unique")
            && !strstr(errmsg, "no such table"))
    {
        result = true;
        cerr << errmsg << endl;
        cerr << "while executing: " << query << endl;
    }
    free(errmsg);
    errmsg = 0;
    return result;
}
