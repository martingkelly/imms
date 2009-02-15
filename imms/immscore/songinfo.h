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
#ifndef __SONGINFO_H
#define __SONGINFO_H

#include <time.h>

#include <string>

#include "immsconf.h"

using std::string;

class InfoSlave
{
public:
    virtual string get_artist() { return ""; }
    virtual string get_title()  { return ""; }
    virtual string get_album()  { return ""; }

    virtual time_t get_length() { return 0; }
    virtual int get_track_num() { return 0; }

    virtual ~InfoSlave() {};
};

class SongInfo : public InfoSlave
{
public:
    SongInfo() : filename(""), myslave(0) { };
    SongInfo(const string &_filename) : myslave(0) { link(_filename); }
    ~SongInfo() { delete myslave; }

    virtual string get_artist();
    virtual string get_title();
    virtual string get_album();
    virtual time_t get_length();
    virtual int get_track_num();

    void link(const string &_filename);

protected:
    string filename;
    InfoSlave *myslave;
};

#endif
