#ifndef __CORRELATE_H
#define __CORRELATE_H

#include <sys/time.h>
#include <string>

#include "immsconf.h"
#include "basicdb.h"

using std::string;

class CorrelationDb : virtual public BasicDb
{
public:
    float correlate(int from);
    void add_recent(int weight);
    void clear_recent() { expire_recent(""); }

    ~CorrelationDb() { clear_recent(); }

protected:
    void expire_recent(const string& where_clause);
    void update_correlation(int from, int to, float weight);
    int expire_recent_callback_1(int argc, char **argv);
    int expire_recent_callback_2(int argc, char **argv);
    int update_secondaty_correlations(int argc, char **argv);

    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0);

private:
    // shared within callbacks
    bool abort_requested;
    int from, from_weight, to, to_weight;
    float weight;

    struct timeval start;
};

#endif
