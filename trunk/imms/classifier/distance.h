#ifndef __DISTANCE_H
#define __DISTANCE_H

#include <analyzer/mfcckeeper.h>

struct EMD {
    static float distance(const MixtureModel &m1, const MixtureModel &m2);
private:
    static float dist(int *f1, int *f2)
        { return cost[*f1][*f2]; }
    static float cost[NUMGAUSS][NUMGAUSS];
};

float song_cepstr_distance(int uid1, int uid2);

#endif
