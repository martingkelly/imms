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
#ifndef __DISTANCE_H
#define __DISTANCE_H

#include <math.h>

#include <stdlib.h>

#include <analyzer/mfcckeeper.h>
#include <analyzer/beatkeeper.h>

struct EMD {
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
