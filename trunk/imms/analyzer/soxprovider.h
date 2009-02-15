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
#include <sstream>
#include <string>

#include <immsutil.h>
#include <strmanip.h>

using std::string;
using std::endl;
using std::ostringstream;

FILE *run_sox(const string &path, int samplerate, bool sign = false)
{
    string epath = rex.replace(path, "'", "'\"'\"'", Regexx::global);
    string extension = string_tolower(path_get_extension(path));

    ostringstream command;
    command << "nice -n 15 sox ";
    if (extension == "mp3" || extension == "ogg")
        command << "-t ." << extension << " ";
    command << "\'" << epath << "\' ";
    command << "-t .raw -2 -c 1 -r " << samplerate
        << (sign ? " -s" : " -u") << " -";
    LOG(INFO) << "Executing: " << command.str() << endl;
    return popen(command.str().c_str(), "r");
}
