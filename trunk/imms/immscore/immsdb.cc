#include <iostream>

#include "strmanip.h"
#include "immsdb.h"

using std::cerr;
using std::endl;

ImmsDb::ImmsDb()
{
    sql_schema_upgrade();
    sql_create_tables();
}

void ImmsDb::sql_create_tables()
{
    BasicDb::sql_create_tables();
    CorrelationDb::sql_create_tables();
    PlaylistDb::sql_create_tables();
}

void ImmsDb::sql_schema_upgrade(int from)
{
    try {
        Q("CREATE TABLE 'Schema' ('version' TEXT NOT NULL, "
                "'description' TEXT UNIQUE NOT NULL);").execute();
    } catch (SQLException &e) {}

    try {
        Q q("SELECT version FROM 'Schema' WHERE description ='latest';");
        if (q.next())
            q >> from;
    }
    WARNIFFAILED();

    if (from > SCHEMA_VERSION)
    {
        cerr << "IMMS: Newer database schema detected." << endl;
        cerr << "IMMS: Please update IMMS!" << endl;
        close_database();
        return;
    }

    if (from == SCHEMA_VERSION)
        return;

    cerr << "IMMS: Outdated database schema detected." << endl;
    cerr << "IMMS: Attempting to update." << endl;
    
    BasicDb::sql_schema_upgrade(from);
    CorrelationDb::sql_schema_upgrade(from);
    PlaylistDb::sql_schema_upgrade(from);

    try {
        Q("INSERT OR REPLACE INTO 'Schema' ('description', 'version') "
                "VALUES ('latest', '" +  itos(SCHEMA_VERSION) + "');").execute();
    }
    WARNIFFAILED();
}
