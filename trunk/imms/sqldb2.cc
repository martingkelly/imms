#include <iostream>
#include <sstream>

#include <sqlite3.h>
#include <sys/stat.h>

#include "strmanip.h"
#include "sqldb2.h"
#include "utils.h"

using std::endl;
using std::cerr;
using std::ostringstream;

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

SqlDb::SqlDb()
{
    mkdir(get_imms_root().c_str(), 0700);
    if (!access((get_imms_root() + "imms.db").c_str(), R_OK)
            && access((get_imms_root() + "imms2.db").c_str(), F_OK))
    {
        cerr << string(60, '*') << endl;
        cerr << "Old database format detected, "
            "will attempt an upgrade..." << endl;
        ostringstream command;
        command << "sqlite " << get_imms_root() << "imms.db .dump | sqlite3 "
            << get_imms_root() << "imms2.db" << endl;
        cerr << "Running: " << command.str() << endl;
        system(command.str().c_str());
        cerr << "If you see errors above verify that you have *both*"
            " sqlite 2.8.x" << endl;
        cerr << "and 3.0.x installed and rerun the command by hand." << endl;
        cerr << string(60, '*') << endl;
    }

    dbcon.open(get_imms_root() + "imms2.db");
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
