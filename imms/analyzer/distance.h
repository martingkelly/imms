#ifndef __DISTANCE_H
#define __DISTANCE_H

#include <math.h>

#include <analyzer/mfcckeeper.h>
#include <analyzer/beatkeeper.h>

struct EMD {
    static float distance(const MixtureModel &m1, const MixtureModel &m2);
    static float distance(float beats1[BEATSSIZE], float beats2[BEATSSIZE]);
    static float raw_distance(const MixtureModel &m1, const MixtureModel &m2);
    static float raw_distance(float beats1[BEATSSIZE], float beats2[BEATSSIZE]);
private:
    static float gauss_dist(int *f1, int *f2)
        { return cost[*f1][*f2]; }
    static float linear_dist(int *f1, int *f2)
        { return abs(*f1 - *f2); }
    static float cost[NUMGAUSS][NUMGAUSS];
};

float song_cepstr_distance(int uid1, int uid2);
float song_bpm_distance(int uid1, int uid2);

#endif
