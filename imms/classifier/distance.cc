#include <assert.h>
#include <iostream>

#include <analyzer/beatkeeper.h>
#include <song.h>

#include "distance.h"
#include "emd.h"

using std::cerr;
using std::endl;

float KL_Distance(const Gaussian &g1, const Gaussian &g2)
{
    float total = 0;
    for (int i = 0; i < NUMCEPSTR; ++i)
    {
        // sanitize the variences so we don't get huge distances
        float var1 = std::max(g1.vars[i], 10.0f);
        float var2 = std::max(g2.vars[i], 10.0f);
        float dist = var1 / var2 + var2 / var1 +
            pow(g1.means[i] - g2.means[i], 2.0f) *
            (1.0f / var1 + 1.0f / var2);

        total += dist - 2;
    }
    return total;
}

float Eucleadian_Distance(const Gaussian &g1, const Gaussian &g2)
{
    float total = 0;
    for (int i = 0; i < NUMCEPSTR; ++i)
        total += pow(g1.means[i] - g2.means[i], 2);
    return sqrt(total);
}

float EMD::cost[NUMGAUSS][NUMGAUSS];

float EMD::distance(const MixtureModel &m1, const MixtureModel &m2)
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

struct TRIV {
    static float distance(const MixtureModel &m1, const MixtureModel &m2)
    {
        float total = 0;
        for (int i = 0; i < NUMGAUSS; ++i)
            for (int j = 0; j < NUMGAUSS; ++j)
                total += Eucleadian_Distance(m1.gauss[i], m2.gauss[j])
                    * m1.gauss[i].weight * m2.gauss[j].weight;
        return total;
    }
};

float song_cepstr_distance(int uid1, int uid2)
{
    Song song1("", uid1), song2("", uid2);

    MixtureModel m1, m2;
    float beats[BEATSSIZE];

    if (!song1.get_acoustic(&m1, sizeof(MixtureModel),
            beats, sizeof(beats)))
    {
        cerr << "warning: failed to load cepstrum data uid " << uid1 << endl;
        return -1;
    }

    if (!song2.get_acoustic(&m2, sizeof(MixtureModel),
            beats, sizeof(beats)))
    {
        cerr << "warning: failed to load cepstrum data uid " << uid2 << endl;
        return -1;
    }

    return EMD::distance(m1, m2);
}
