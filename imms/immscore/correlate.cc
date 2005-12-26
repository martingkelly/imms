#include <assert.h>
#include <math.h>
#include <time.h>
#include <iostream>

#include "flags.h"
#include "correlate.h"
#include "strmanip.h"
#include "immsutil.h"

using std::endl;
using std::cerr;

#define CORRELATION_TIME    (15*30)   // n * 30 ==> n minutes
#define MAX_CORR_STR        "12"
#define MAX_CORRELATION     12
#define SECOND_DEGREE       0.5
#define PROCESSING_TIME     5000000

CorrelationDb::CorrelationDb() : correlate_from(time(0))
{
    gettimeofday(&start, 0);
}

void CorrelationDb::sql_create_tables()
{
    RuntimeErrorBlocker reb;
    QueryCacheDisabler qcd;
    try {
        Q("CREATE TABLE C.Correlations ("
                "'x' INTEGER NOT NULL, "
                "'y' INTEGER NOT NULL, "
                "'weight' INTEGER DEFAULT '0');").execute();

        Q("CREATE TEMP TABLE TmpCorr ("
                "'x' INTEGER NOT NULL, "
                "'y' INTEGER NOT NULL, "
                "'weight' INTEGER DEFAULT '0');").execute();

        Q("CREATE UNIQUE INDEX C.Correlations_x_y_i "
                "ON Correlations (x, y);").execute();

        Q("CREATE INDEX C.Correlations_x_i ON Correlations (x);").execute();
        Q("CREATE INDEX C.Correlations_y_i ON Correlations (y);").execute();
    }
    WARNIFFAILED();
}

void CorrelationDb::add_recent(int uid, time_t skipped_at, int flags)
{
    if (uid > -1)
    {
        try {
            Q q("INSERT INTO Journal VALUES (?, ?, ?, ?);");
            q << uid << skipped_at << flags << time(0);
            q.execute();
        }
        WARNIFFAILED();
    }
}

void CorrelationDb::get_related(vector<int> &out, int pivot_sid, int limit)
{
    string query =
        "SELECT pos FROM Filter NATURAL INNER JOIN Library "
            "WHERE sid IN ("
            "SELECT L.sid FROM C.Correlations AS C INNER JOIN Last AS L "
                "ON CASE WHEN C.x = ? THEN C.y ELSE C.x END = L.sid "
            "WHERE C.weight > 0 AND (C.x = ? OR C.y = ?) AND L.last > ? "
            "ORDER BY C.weight DESC LIMIT " + itos(limit) + ");";

    try {
        Q q(query);

        q << pivot_sid << pivot_sid << pivot_sid << (time(0) - HOUR);

        while (q.next())
        {
            int pos;
            q >> pos;
            out.push_back(pos);
        }
    }
    WARNIFFAILED();
}

void CorrelationDb::maybe_expire_recent()
{
    struct timeval now;
    gettimeofday(&now, 0);

    if (usec_diff(start, now) < 10000000)
        return;

    start = now;

    expire_recent(time(0) - CORRELATION_TIME);
}

void CorrelationDb::expire_recent(time_t cutoff)
{
#if 0 && defined(DEBUG)
    cerr << "Running expire recent..." << endl;
    StackTimer t;
#endif

    try {
        AutoTransaction a;

        while (1)
        {
            Q q("SELECT Library.sid, Journal.played, "
                    "Journal.flags, Journal.time "
                    "FROM Journal INNER JOIN Library "
                    "ON Journal.uid = Library.uid "
                    "WHERE Journal.time > ? ORDER BY Journal.time ASC;");
            q << correlate_from;

            if (!q.next())
                break;

            int flags;
            time_t next, played;

            q >> from >> played >> flags >> next;
            from_weight = Flags::deltify(played, flags);

            if (next > cutoff) 
                break;

            correlate_from = next + 1;

            if (from_weight == -1)
                continue;

            while (q.next())
            {
                q >> to >> played >> flags;
                to_weight = Flags::deltify(played, flags);
                expire_recent_helper();
            }
        }

        a.commit();
    }
    WARNIFFAILED();
}

void CorrelationDb::expire_recent_helper()
{
    if (to == from || to_weight == -1)
        return;

    if (from_weight < 0 && to_weight < 0)
        return;

#ifdef DEBUG
    //cerr << string(55, '-') << endl;
    cerr << "processing update between " << std::min(from, to) <<
        " and " << std::max(from, to) << endl;
#endif

    weight = sqrt(abs(from_weight * to_weight));

    if (from_weight < 0 || to_weight < 0)
        weight = -weight;

    // Update the primary link
    update_correlation(from, to, weight);

    struct timeval now;
    gettimeofday(&now, 0);

    if (usec_diff(start, now) > PROCESSING_TIME || fabs(weight) < 2)
        return;
    
    try {
        Q("DELETE FROM TmpCorr;").execute();
    } IGNOREFAILURE();

    {
        string query("INSERT INTO TmpCorr SELECT x, y, weight "
            "FROM C.Correlations "
            "WHERE (x IN (?, ?) OR y IN (?, ?)) AND ");
        query += (weight > 0 ? string("abs") : string("")) + " (weight) > 1;";

        Q q(query);
        q << to << from << to << from;
        q.execute();
    }

    {
        Q q("SELECT * FROM TmpCorr;");
        while (q.next())
        {
            int node1, node2;
            float outer;
            q >> node1 >> node2 >> outer;
            update_secondary_correlations(node1, node2, outer);
        }
    }
}

void CorrelationDb::update_secondary_correlations(int node1, int node2,
        float outer)
{
    // Don't update the primary link again
    if ((node1 == to && node2 == from) || (node1 == from && node2 == to))
        return;

    node1 = (node1 == to ? from : (node1 == from) ? to : node1);
    node2 = (node2 == to ? from : (node2 == from) ? to : node2);

    float scale = outer * SECOND_DEGREE / MAX_CORRELATION;
    update_correlation(node1, node2, weight * scale);

    return;
}

void CorrelationDb::update_correlation(int from, int to, float weight)
{
    if (fabs(weight) < 0.25)
        return;

#if defined(DEBUG) && 0
    cerr << " >> Updating link from " << std::min(from, to) << " to "
        << std::max(from, to) << " by " << weight << endl;
#endif

    int min = std::min(from, to), max = std::max(from, to);

    try {
        Q q("INSERT INTO C.Correlations "
                "('x', 'y', 'weight') VALUES (?, ?, ?);");
        q << min << max << weight;
        q.execute();
        return;
    } IGNOREFAILURE();

    {
        Q q("UPDATE C.Correlations SET weight = "
                "max(min(weight + ?, " MAX_CORR_STR "), -" MAX_CORR_STR ") "
                "WHERE x = ? AND y = ?;");
        q << weight << min << max;
        q.execute();
    }
}

float CorrelationDb::correlate(int sid1, int sid2)
{
    if (sid1 < 0 || sid2 < 0)
        return 0;

    int min = std::min(sid1, sid2), max = std::max(sid1, sid2);
    float correlation = 0;
    
    try {
        Q q("SELECT weight FROM C.Correlations WHERE x = ? AND y = ?;");
        q << min << max;

        if (q.next())
            q >> correlation;
    }
    WARNIFFAILED();

    return correlation;
}

void CorrelationDb::sql_schema_upgrade(int from)
{
    RuntimeErrorBlocker reb;
    QueryCacheDisabler qcd;
    try {
        if (from < 6)
        {
            // Backup the existing tables
            Q("CREATE TEMP TABLE Correlations_backup "
                    "AS SELECT * FROM Correlations;").execute();
            Q("DROP TABLE Correlations;").execute();

            // Create new tables
            sql_create_tables();

            // Copy the data into new tables, and drop the backups
            try {
                Q("INSERT OR REPLACE INTO Correlations (x, y, weight) "
                        "SELECT origin, destination, weight "
                        "FROM Correlations_backup;").execute();
            }
            WARNIFFAILED();
            Q("DROP TABLE Correlations_backup;").execute();
        }
        if (from < 10)
        {
            sql_create_tables();

            Q("DELETE FROM Correlations WHERE abs(weight) < 0.25;").execute();

            Q("INSERT INTO C.Correlations "
                    "SELECT * FROM Correlations;").execute();
            Q("DROP TABLE Correlations;").execute();

            Q("VACUUM;").execute();
        }
    }
    WARNIFFAILED();
}
