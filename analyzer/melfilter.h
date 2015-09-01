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
#ifndef __MELFILTER_H
#define __MELFILTER_H

#include <vector>
#include <string>

using std::vector;

#define NUMMEL      40  // rough guess within guidelines,
                        // to match sampling rate but leave space for smoothing

// A triangular shaped bandpass filter.
// Filters frequencies within a range to help calculate MFCCs.
//
// For an illustration and example see
// http://cmusphinx.sourceforge.net/sphinx4/javadoc/edu/cmu/sphinx/frontend/frequencywarp/MelFilter.html
class MelFilter
{
public:
    MelFilter(int leftf, int centerf, int rightf);
    double apply(const vector<double> &data);
    string print();
protected:
    int start;
    vector<float> weights;
};

// Class to store a number of mel-filters
// so they can be applied in series to the whole song/input.
//
// For an illustration and example see
// http://cmusphinx.sourceforge.net/sphinx4/javadoc/edu/cmu/sphinx/frontend/frequencywarp/MelFrequencyFilterBank.html
class MelFilterBank
{
public:
    MelFilterBank();
    void apply(const vector<double> &data, vector<double> &mfc);
    string print();
protected:
    vector<MelFilter> filters;
};

#endif
