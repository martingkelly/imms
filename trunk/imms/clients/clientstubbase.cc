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
#include <string.h>
#include <unistd.h>

#include <sstream>
#include <iostream>

#include "clientstubbase.h"
#include "appname.h"
#include "immsutil.h"

using std::ostringstream;
using std::cerr;
using std::endl;

const std::string AppName = CLIENT_APP;

void IMMSClientStub::setup(bool use_xidle)
{
    ostringstream osstr;
    osstr << "Setup " << use_xidle;
    write_command(osstr.str());
}
void IMMSClientStub::start_song(int position, std::string path)
{
    ostringstream osstr;
    osstr << "StartSong " << position << " " << path;
    write_command(osstr.str());
}
void IMMSClientStub::end_song(bool at_the_end, bool jumped, bool bad)
{
    ostringstream osstr;
    osstr << "EndSong " << at_the_end << " " << jumped << " " << bad;
    write_command(osstr.str());
}
void IMMSClientStub::select_next() { write_command("SelectNext"); }
void IMMSClientStub::playlist_changed(int length)
{
#ifdef DEBUG
    LOG(ERROR) << "sending out pl len = " << length << endl;
#endif

    ostringstream osstr;
    osstr << "PlaylistChanged " << length;
    write_command(osstr.str());
}
