#include <assert.h>
#include <iostream>

#include <song.h>
#include <immsutil.h>

#include "distance.h"
#include "emd.h"

using std::cerr;
using std::endl;

float KL_Divergence(const Gaussian &g1, const Gaussian &g2)
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
            cost[i][j] = KL_Divergence(m1.gauss[i], m2.gauss[j]);
    }

    signature_t s1 = { NUMGAUSS, features, w1 };
    signature_t s2 = { NUMGAUSS, features, w2 };
    return emd(&s1, &s2, EMD::gauss_dist, 0, 0);
}

static bool normalize(float beats[BEATSSIZE], int comb)
{
    float sum = 0, min = 1e100;

    for (int i = 0; i < BEATSSIZE; ++i)
    {
        sum += beats[i];
        if (beats[i] < min)
            min = beats[i];
    }

    if (sum == 0)
        return false;

    // scale to keep the total area under the curve to be fixed
    float scale = 100.0 / sum;
    for (int i = 0; i < BEATSSIZE; ++i)
    {
        float cur = beats[i];
        beats[i] = 0;
        beats[i / comb] += cur * scale;
    }

    return true;
}

float EMD::distance(float beats1[BEATSSIZE], float beats2[BEATSSIZE])
{
    static const int comb = 3;
    feature_t features[BEATSSIZE / comb];

    for (int i = 0; i < BEATSSIZE / comb; ++i)
        features[i] = i;

    if (!normalize(beats1, comb))
        return -1;
    if (!normalize(beats2, comb))
        return -1;

    signature_t s1 = { BEATSSIZE / comb, features, beats1 };
    signature_t s2 = { BEATSSIZE / comb, features, beats2 };
    return emd(&s1, &s2, EMD::linear_dist, 0, 0);
}

float song_cepstr_distance(int uid1, int uid2)
{
    Song song1("", uid1), song2("", uid2);

    MixtureModel m1, m2;
    float beats[BEATSSIZE];

    if (!song1.get_acoustic(&m1, sizeof(MixtureModel),
            beats, sizeof(beats)))
    {
        LOG(ERROR) << "warning: failed to load cepstrum data for uid "
            << uid1 << endl;
        return -1;
    }

    if (!song2.get_acoustic(&m2, sizeof(MixtureModel),
            beats, sizeof(beats)))
    {
        LOG(ERROR) << "warning: failed to load cepstrum data for uid "
            << uid2 << endl;
        return -1;
    }

    return EMD::distance(m1, m2);
}

float song_bpm_distance(int uid1, int uid2)
{
    Song song1("", uid1), song2("", uid2);

    MixtureModel m;
    float beats1[BEATSSIZE], beats2[BEATSSIZE];

    if (!song1.get_acoustic(&m, sizeof(MixtureModel),
            beats1, sizeof(beats1)))
    {
        LOG(ERROR) << "warning: failed to load bpm data for uid "
            << uid1 << endl;
        return -1;
    }

    if (!song2.get_acoustic(&m, sizeof(MixtureModel),
            beats2, sizeof(beats2)))
    {
        LOG(ERROR) << "warning: failed to load bpm data for uid "
            << uid2 << endl;
        return -1;
    }

    return EMD::distance(beats1, beats2);
}
