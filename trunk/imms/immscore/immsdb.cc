/*
 IMMS: Intelligent Multimedia Management System
 Copyright (C) 2001-2009 Michael Grigoriev

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include <iostream>

#include "strmanip.h"
#include "immsdb.h"

using std::cerr;
using std::endl;

ImmsDb::ImmsDb()
{
    sql_schema_upgrade(0);
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
        Q("CREATE TABLE 'Schema' ( "
                "'description' TEXT UNIQUE NOT NULL, "
                "'version' TEXT NOT NULL);").execute();
    } catch (SQLException &e) {}

    try {
        Q q("SELECT version FROM 'Schema' WHERE description = 'latest';");
        if (q.next())
            q >> from;
        else
            from = 12;  // Hack to get around broken upgrades. Remove later.
    }
    WARNIFFAILED();

    if (from > SCHEMA_VERSION)
    {
        cerr << "IMMS: Newer database schema detected." << endl;
        cerr << "IMMS: Please update IMMS!" << endl;
        close_database();
        return;
    }

    if (from && from < 12)
    {
        cerr << "IMMS: Database version too old." << endl;
        cerr << "IMMS: Upgrade to an IMMS 3.0.x first, "
                "or delete you .imms directory." << endl;
        close_database();
        return;
    }

    if (from && from != SCHEMA_VERSION) {
        cerr << "IMMS: Outdated database schema detected." << endl;
        cerr << "IMMS: Attempting to update." << endl;

        BasicDb::sql_schema_upgrade(from);
        CorrelationDb::sql_schema_upgrade(from);
        PlaylistDb::sql_schema_upgrade(from);
    }

    try {
        Q("INSERT OR REPLACE INTO 'Schema' ('description', 'version') "
                "VALUES ('latest', '" +  itos(SCHEMA_VERSION) + "');").execute();
    }
    WARNIFFAILED();
}
