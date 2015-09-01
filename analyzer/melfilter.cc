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

#include <cmath>
#include <sstream>
#include <iostream>

#include <immsutil.h>
#include "analyzer.h"
#include "melfilter.h"

using std::ostringstream;
using std::endl;
using std::cerr;

inline int freq2mel(int f)
    { return ROUND(2595.0 * log10(1 + f/700.0)); }
inline int mel2freq(int m)
    { return ROUND(700.0 * (pow(10.0, m / 2595.0) - 1)); }
inline int freq2indx(int i)
    { return ROUND(i * NUMFREQS / MAXFREQ); }

MelFilter::MelFilter(int leftf, int centerf, int rightf)
{
    int lefti = freq2indx(leftf);
    int centeri = freq2indx(centerf);
    int righti = freq2indx(rightf);

    start = lefti;
    int size = righti - lefti;
    weights.resize(size);

    // total weight of the filter should add up to 1
    float height = 2.0f / size;

    int leftisize = centeri - lefti;
    int rightisize = righti - centeri;

    float lslope = height / (centeri - lefti);
    float rslope = height / (righti - centeri);

    for (int i = 0; i < leftisize; ++i)
        weights[i] = lslope * i;

    for (int i = 0; i < rightisize; ++i)
        weights[leftisize + i] = height - rslope * i;

    weights[leftisize] = height;
}

double MelFilter::apply(const vector<double> &data)
{
    double power = 0;
    for (unsigned i = 0; i < weights.size(); ++i)
    {
        assert(start + i < data.size());
        power += data[start + i] * weights[i];
    }
    return power;
}

string MelFilter::print()
{
    ostringstream s;
    for (int i = 0; i < start; ++i) 
        s << "0 ";
    for (unsigned i = 0; i < weights.size(); ++i) 
        s << weights[i] << " ";
    return s.str();
}

MelFilterBank::MelFilterBank()
{
    int minmel = freq2mel(MINFREQ);
    int maxmel = freq2mel(MAXFREQ);
    int meldelta = (maxmel - minmel) / (NUMMEL + 1);

    for (int cur = minmel + meldelta;
            cur + meldelta < maxmel; cur += meldelta)
    {
        int leftf = mel2freq(cur - meldelta);
        int midf = mel2freq(cur);
        int rightf = mel2freq(cur + meldelta);

        MelFilter f(leftf, midf, rightf);
        filters.push_back(f);
    }

#if defined(DEBUG) && 0
    cerr << print();
#endif
}

void MelFilterBank::apply(const vector<double> &data, vector<double> &mfc)
{
    mfc.clear();
    for (vector<MelFilter>::iterator i = filters.begin();
            i != filters.end(); ++i)
        mfc.push_back(i->apply(data));
}

string MelFilterBank::print()
{
    ostringstream s;
    for (unsigned i = 0; i < filters.size(); ++i)
        s << i << ": " << filters[i].print() << endl;
    return s.str();
}
