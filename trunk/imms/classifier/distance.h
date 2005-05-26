#ifndef __DISTANCE_H
#define __DISTANCE_H

#include <assert.h>

#include <analyzer/mfcckeeper.h>

#include "emd.h"

float KL_Distance(const Gaussian &g1, const Gaussian &g2)
{
    float total = 0;
    for (int i = 0; i < NUMCEPSTR; ++i)
    {
        // sanitize the variences so we don't get huge distances
        float var1 = max(g1.vars[i], 10.0);
        float var2 = max(g2.vars[i], 10.0);
        float dist = var1 / var2 + var2 / var1 +
            pow(g1.means[i] - g2.means[i], 2.0) *
            (1.0 / var1 + 1.0 / var2);

        total += dist - 2;
    }
    return total;
}

struct EMD {
    static float distance(const MixtureModel &m1, const MixtureModel &m2)
    {
        feature_t features[NUMGAUSS];
        float w1[NUMGAUSS], w2[NUMGAUSS];

        for (int i = 0; i < NUMGAUSS; ++i)
        {
            features[i] = i;
            w1[i] = m1.gauss[i].weight;
            w2[i] = m2.gauss[i].weight;

            for (int j = 0; j < NUMGAUSS; ++j)
                cost[i][j] = KL_Distance(m1.gauss[i], m2.gauss[j]);
        }

        signature_t s1 = { NUMGAUSS, features, w1 };
        signature_t s2 = { NUMGAUSS, features, w2 };
        return emd(&s1, &s2, EMD::dist, 0, 0);
    }
private:
    static float dist(feature_t *f1, feature_t *f2)
    {
        assert(*f1 < NUMGAUSS && *f2 < NUMGAUSS);
        return cost[*f1][*f2];
    }
    static float cost[NUMGAUSS][NUMGAUSS];
};

#endif
