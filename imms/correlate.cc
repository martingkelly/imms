#include <assert.h>
#include <iostream>

#include "correlate.h"
#include "strmanip.h"
#include "utils.h"

using std::endl;
using std::cerr;

#define CORRELATION_TIME    (12*30)   // n * 30 ==> n minutes
#define MAX_CORR_STR        "10"
#define MAX_CORRELATION     10
#define SECOND_DEGREE       0.5

static inline string mkkey(int i, int j)
    { return itos(std::min(i, j)) + "|" + itos(std::max(i, j)); }

static inline string mkkey(string i, string j)
    { return i + "|" + j; }

void CorrelationDb::sql_create_tables()
{
    run_query(
            "CREATE TABLE 'Correlations' ("
            "'key' VARCHAR(13) UNIQUE NOT NULL, "
            "'origin' INTEGER NOT NULL, "
            "'destination' INTEGER NOT NULL, "
            "'weight' INTEGER DEFAULT '0');");

    run_query(
            "CREATE TEMPORARY TABLE 'Recent' ("
            "'sid' INTEGER NOT NULL, "
            "'weight' INTEGER NOT NULL, "
            "'time' TIMESTAMP);");
}

void CorrelationDb::add_recent(int weight)
{
    expire_recent("WHERE time < '" + itos(time(0) - CORRELATION_TIME) + "'");

    if (sid > -1)
        run_query("INSERT INTO 'Recent' "
                "VALUES ('" + itos(sid) + "', '"
                + itos(weight) + "', '"
                + itos(time(0)) + "');");
}

void CorrelationDb::expire_recent(const string &where_clause)
{
    abort_requested = false;
    gettimeofday(&start, 0);

    ImmsCallback<CorrelationDb> callback(this,
            &CorrelationDb::expire_recent_callback_1);

    select_query("SELECT sid, weight FROM 'Recent' " + where_clause + ";",
             &callback, 2);

#ifdef DEBUG
    struct timeval now;
    gettimeofday(&now, 0);
    int msec = usec_diff(start, now) / 1000;
    cerr << "Update took " << msec << " microseconds." << endl;
#endif
}

int CorrelationDb::expire_recent_callback_1(int argc, char **argv)
{
    assert(argc == 2);
    if (abort_requested)
        return 4;

    from = atoi(argv[0]);
    from_weight = atoi(argv[1]);

    run_query("DELETE FROM 'Recent' WHERE sid = '" + itos(from) + "';");

    ImmsCallback<CorrelationDb> callback(this,
            &CorrelationDb::expire_recent_callback_2);

    select_query("SELECT sid, weight FROM 'Recent';", &callback);
    return 0;
}

int CorrelationDb::expire_recent_callback_2(int argc, char **argv)
{
    assert(argc == 2);

    to = atoi(argv[0]);
    to_weight = atoi(argv[1]);

    if (to == from)
        return 0;

    if (from_weight < 0 && to_weight < 0)
        return 0;

    struct timeval now;
    gettimeofday(&now, 0);
    if ((abort_requested = usec_diff(start, now) > 2 * 1000000))
        return 4; // SQLITE_ABORT

#ifdef DEBUG
    cerr << string(55, '-') << endl;
    cerr << "processing update between " << std::min(from, to) <<
        " and " << std::max(from, to) << endl;
#endif

    weight = sqrt(abs(from_weight * to_weight));

    if (from_weight < 0 || to_weight < 0)
        weight = -weight;

    // Update the primary link
    update_correlation(from, to, weight);

    if (fabs(weight) < 3)
        return 0;

    ImmsCallback<CorrelationDb> callback(this,
            &CorrelationDb::update_secondaty_correlations);

    // Update secondary links
    select_query(
            "SELECT origin,destination,weight FROM 'Correlations' "
            "WHERE ("
                    "origin = '" + itos(to) + "' "
                    "OR origin = '" + itos(from) + "' "
                    "OR destination = '" + itos(to) + "' "
                    "OR destination = '" + itos(from) + "'"
                ") AND " + (weight > 0 ? "abs" : "") + " (weight) > 1;",
                &callback, 3);

    return 0;
}

int CorrelationDb::update_secondaty_correlations(int argc, char **argv)
{
    assert(argc == 3);

    int node1 = atoi(argv[0]), node2 = atoi(argv[1]);

    // Don't update the primary link again
    if ((node1 == to && node2 == from) || (node1 == from && node2 == to))
        return 0;

    node1 = (node1 == to ? from : (node1 == from) ? to : node1);
    node2 = (node2 == to ? from : (node2 == from) ? to : node2);

    float scale = atof(argv[2]) * SECOND_DEGREE / MAX_CORRELATION;
    update_correlation(node1, node2, weight * scale);

    return 0;
}

void CorrelationDb::update_correlation(int from, int to, float weight)
{
#ifdef DEBUG
    cerr << " >> Updating link from " << std::min(from, to) << " to "
        << std::max(from, to) << " by " << weight << endl;
#endif

    string min = itos(std::min(from, to)), max = itos(std::max(from, to));

    if (run_query("INSERT INTO 'Correlations' "
            " ('key', 'origin', 'destination', 'weight') "
                "VALUES ('" + mkkey(min, max) + "', '" + min +
                "', '" + max + "', '" + itos(weight) + "');"))
        return;

    run_query(
        "UPDATE 'Correlations' SET weight = "
             "max(min(weight + '" + itos(weight) + "', "
              MAX_CORR_STR "), -" MAX_CORR_STR ") "
        "WHERE key = '" + mkkey(min, max) + "';");
}

float CorrelationDb::correlate(int from)
{
    if (sid < 0)
        return 0;
    
    select_query(
            "SELECT weight FROM 'Correlations' "
            "WHERE key = '" + mkkey(from, sid) + "';");

    return nrow && resultp[1] ? atof(resultp[1]) : 0;
}

void CorrelationDb::sql_schema_upgrade(int from)
{
    if (from < 5)
    {
        // Backup the existing tables
        run_query("CREATE TEMP TABLE Correlations_backup "
                "AS SELECT * FROM Correlations;");
        run_query("DROP TABLE Correlations;");

        // Create new tables
        sql_create_tables();

        // Copy the data into new tables, and drop the backups
        run_query("INSERT OR REPLACE INTO 'Correlations' "
                "SELECT origin||'|'||destination,origin,destination,weight "
                "FROM 'Correlations_backup';");
        run_query("DROP TABLE Correlations_backup;");
    }
}
