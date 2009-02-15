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
#ifndef __HANNING_H
#define __HANNING_H

#include <assert.h>
#include <vector>

class HanningWindow
{
public:
    HanningWindow(unsigned size)
    {
        weights.resize(size);

        static const float alpha = 0.46;
        for (unsigned i = 0; i < size; ++i)
            weights[i] = (1 - alpha) -
                (alpha * cos((2 * M_PI * i)/(size - 1)));
    }
    void apply(double *data, unsigned size)
    {
        assert(size == weights.size());
        for (unsigned i = 0; i < size; ++i)
            data[i] *= weights[i];
    }
private:
    std::vector<double> weights;
};

#endif  // __HANNING_H
