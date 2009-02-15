/*
 IMMS: Intelligent Multimedia Management System
 Copyright (C) Agner Fog

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
#include <math.h>
#include <stdlib.h>

#include "immsutil.h"

inline double Random() { return imms_random(RAND_MAX) / (double)RAND_MAX; }

inline double normal(double m, double s)
{
    // normal distribution with mean m and standard deviation s
    // first random coordinate (normal_x2 is member of class)
    static double normal_x1, normal_x2;
    static bool normal_x2_valid;
    double w;            // radius

    if (normal_x2_valid)
    {
        // we have a valid result from last call
        normal_x2_valid = false;
        return normal_x2 * s + m;
    }

    // make two normally distributed variates by Box-Muller transformation
    do {
        normal_x1 = 2. * Random() - 1.;
        normal_x2 = 2. * Random() - 1.;
        w = normal_x1 * normal_x1 + normal_x2 * normal_x2;
    } while (w >= 1. || w < 1E-30);

    w = sqrt(log(w)*(-2./w));
    // normal_x1 and normal_x2 are independent normally distributed variates
    normal_x1 *= w;
    normal_x2 *= w;    
    normal_x2_valid = true;
    return normal_x1 * s + m;
}    
