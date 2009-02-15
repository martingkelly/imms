/*
 IMMS: Intelligent Multimedia Management System
 Copyright (C) 2001-2009 Michael Grigoriev

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include <assert.h>
#include <iostream>

#include <song.h>
#include <immsutil.h>
#include <string.h>

#include "distance.h"
#include "emd.h"

using std::cerr;
using std::endl;

float KL_Divergence(const Gaussian &g1, const Gaussian &g2)
{
    float total = 0;
    for (int i = 0; i < Gaussian::NumDimensions; ++i)
    {
        // Enforce a minimum for variences so we don't get huge distances
        const float MinVariance = 10.0f;
        float var1 = std::max(g1.vars[i], MinVariance);
        float var2 = std::max(g2.vars[i], MinVariance);
        float dist = var1 / var2 + var2 / var1 +
            pow(g1.means[i] - g2.means[i], 2.0f) *
            (1.0f / var1 + 1.0f / var2);

        total += dist - 2;
    }
    return total;
}

float EMD::cost[NUMGAUSS][NUMGAUSS];

float EMD::raw_distance(const MixtureModel &m1, const MixtureModel &m2)
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

static bool normalize_beat_graph(float beats[BEATSSIZE], float *output, int comb)
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
        output[i / comb] += beats[i] * scale;

    return true;
}

float EMD::raw_distance(float beats1[BEATSSIZE], float beats2[BEATSSIZE])
{
    static const int comb = 5;
    static const int OUTSIZE = DIVROUNDUP(BEATSSIZE, comb);

    feature_t features[OUTSIZE];

    for (int i = 0; i < OUTSIZE; ++i)
        features[i] = i;

    float b1[OUTSIZE], b2[OUTSIZE];
    memset(b1, 0, sizeof(b1));
    memset(b2, 0, sizeof(b2));

    if (!normalize_beat_graph(beats1, b1, comb))
        return -1;
    if (!normalize_beat_graph(beats2, b2, comb))
        return -1;

    signature_t s1 = { OUTSIZE, features, b1 };
    signature_t s2 = { OUTSIZE, features, b2 };
    return emd(&s1, &s2, EMD::linear_dist, 0, 0);
}

float song_cepstr_distance(int uid1, int uid2)
{
    Song song1("", uid1), song2("", uid2);

    MixtureModel m1, m2;

    if (!song1.get_acoustic(&m1, 0))
    {
        LOG(ERROR) << "warning: failed to load cepstrum data for uid "
            << uid1 << endl;
        return -1;
    }

    if (!song2.get_acoustic(&m2, 0))
    {
        LOG(ERROR) << "warning: failed to load cepstrum data for uid "
            << uid2 << endl;
        return -1;
    }

    return EMD::raw_distance(m1, m2);
}

float song_bpm_distance(int uid1, int uid2)
{
    Song song1("", uid1), song2("", uid2);

    MixtureModel m;
    float beats1[BEATSSIZE], beats2[BEATSSIZE];

    if (!song1.get_acoustic(&m, beats1))
    {
        LOG(ERROR) << "warning: failed to load bpm data for uid "
            << uid1 << endl;
        return -1;
    }

    if (!song2.get_acoustic(&m, beats2))
    {
        LOG(ERROR) << "warning: failed to load bpm data for uid "
            << uid2 << endl;
        return -1;
    }

    return EMD::raw_distance(beats1, beats2);
}
