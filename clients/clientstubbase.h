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
#ifndef __CLIENTSTUBBASE_H_
#define __CLIENTSTUBBASE_H_

#include <string>
#include <cerrno>

using std::string;

class IMMSClientStub
{
public:
    virtual ~IMMSClientStub() {}
    void setup(bool use_xidle);
    void start_song(int position, std::string path);
    void end_song(bool at_the_end, bool jumped, bool bad);
    void select_next();
    void playlist_changed(int length);
protected:
    virtual void write_command(const string &line) = 0;
}; 

#endif
