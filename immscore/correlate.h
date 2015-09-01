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
#ifndef __CORRELATE_H
#define __CORRELATE_H

#include <sys/time.h>
#include <string>
#include <vector>
#include <climits>

#include "immsconf.h"
#include "basicdb.h"

using std::string;

class CorrelationDb : virtual public BasicDb
{
public:
    CorrelationDb();

    float correlate(int sid1, int sid2);
    void add_recent(int uid, time_t skipped_at, int flags);
    void clear_recent() { expire_recent(INT_MAX); }
    void expire_recent(time_t cutoff);
    void maybe_expire_recent();

protected:
    void update_correlation(int from, int to, float weight);
    void expire_recent_helper();
    void update_secondary_correlations(int from, int to, float outer);

    void get_related(std::vector<int> &out, int pivot_sid, int limit);

    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0) {}

private:
    // shared within callbacks
    time_t correlate_from;
    int from, from_weight, to, to_weight;
    float weight;
    struct timeval start;
};

#endif
