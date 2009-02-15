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
#ifndef __BEATKEEPER_H
#define __BEATKEEPER_H

#include <string>
#include <vector>

#include "analyzer.h"

#define MINBPM          50
#define MAXBPM          250

#define MINBEATLENGTH   (WINPERSEC*60/MAXBPM)
#define MAXBEATLENGTH   (WINPERSEC*60/MINBPM)
#define BEATSSIZE       (MAXBEATLENGTH-MINBEATLENGTH)

#define OFFSET2BPM(offset)  \
    ROUND(60 * WINPERSEC / (float)(MINBEATLENGTH + offset))

class BeatKeeper
{
    friend class BeatManager;
public:
    BeatKeeper() { reset(); }
    void reset();
    void process(float power);

    void dump(const std::string &filename);
    const BeatKeeper &operator +=(const BeatKeeper &other);

    static bool extract_features(float *beats, std::vector<float> &features);

protected:
    void process_window();

    long unsigned int samples;
    float average_with, *last_window, *current_window, *current_position;
    float data[2*MAXBEATLENGTH];
    float beats[BEATSSIZE];
};

class BeatManager
{
public:
    void process(const std::vector<double> &melfreqs);
    void finalize();

    float *get_result();

    static const int ResultSize = BEATSSIZE * sizeof(float);
protected:
    BeatKeeper lofreq;
};

#endif
