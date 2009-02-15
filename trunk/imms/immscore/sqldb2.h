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
#ifndef __SQLDB_H
#define __SQLDB_H

#include <string>
#include <memory>
#include "sqlite++.h"

using std::string;
using std::auto_ptr;

class SqlDb
{
public:
    SqlDb();
    ~SqlDb();

protected:
    void close_database();
    int changes();

private:
    auto_ptr<AttachedDatabase> correlations, acoustic;
    SQLDatabaseConnection dbcon;
};

#endif
