#ifndef __CORRELATE_H
#define __CORRELATE_H

#include <sys/time.h>
#include <string>
#include <vector>

#include "immsconf.h"
#include "basicdb.h"

using std::string;

class CorrelationDb : virtual public BasicDb
{
public:
    CorrelationDb();

    float correlate(int sid1, int sid2);
    void add_recent(int uid, int delta);
    void clear_recent() { expire_recent(INT_MAX); }
    void expire_recent(time_t cutoff);
    void maybe_expire_recent();

protected:
    void update_correlation(int from, int to, float weight);
    void expire_recent_helper();
    void update_secondary_correlations(int from, int to, float outer);

    void get_related(std::vector<int> &out, int pivot_sid, int limit);

    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0);

private:
    // shared within callbacks
    time_t correlate_from;
    int from, from_weight, to, to_weight;
    float weight;
    struct timeval start;
};

#endif
