#ifndef __CLUSTER_H
#define __CLUSTER_H

#include "sqlite++.h"

class AcousticCluster
{
public:
    AcousticCluster();
    void calculate_distances(int from = -1);
protected:
    void create_sql_tables();

    AttachedDatabase ad;
};

#endif
