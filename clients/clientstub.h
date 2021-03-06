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
#ifndef __CLIENTSTUB_H_
#define __CLIENTSTUB_H_

#include "giosocket.h"
#include "immsutil.h"
#include "clientstubbase.h"

#include <stdlib.h>
#include <errno.h>

#include <sstream>
#include <iostream>

using std::stringstream;
using std::ostringstream;
using std::cerr;
using std::endl;

template <typename Ops>
class IMMSClient : public IMMSClientStub, protected GIOSocket 
{
public:
    IMMSClient() : connected(false) { }
    bool connect()
    {
        int fd = socket_connect(get_imms_root("socket"));
        if (fd > 0)
        {
            init(fd);
            connected = true;
            write_command("IMMS");
            return true;
        }
        LOG(ERROR) << "Connection failed: " << strerror(errno) << endl;
        return false;
    }
    virtual void write_command(const string &line)
        { if (isok()) GIOSocket::write(line + "\n"); }
    virtual void process_line(const string &line)
    {
        stringstream sstr;
        sstr << line;

#if defined(DEBUG) && 1
        std::cout << "< " << line << endl;
#endif

        string command = "";
        sstr >> command;

        if (command == "ResetSelection")
        {
            Ops::reset_selection();
            return;
        }
        if (command == "TryAgain")
        {
            write_command("SelectNext");
            return;
        }
        if (command == "EnqueueNext")
        {
            int next;
            sstr >> next;
            Ops::set_next(next);
            return;
        }
        if (command == "PlaylistChanged")
        {
            IMMSClientStub::playlist_changed(Ops::get_length());
            return;
        }
        if (command == "GetPlaylistItem")
        {
            int i;
            sstr >> i;
            send_item("PlaylistItem", i);
            return;
        }
        if (command == "GetEntirePlaylist")
        {
            for (int i = 0; i < Ops::get_length(); ++i)
                send_item("Playlist", i);
            write_command("PlaylistEnd");
            return;
        }

        LOG(ERROR) << "Unknown command: " << command << endl;
    }
    virtual void connection_lost() { connected = false; }

    bool check_connection()
    {
        if (isok())
            return true;

        system("nice -n 5 immsd &");

        return connect();
    }
    
    bool isok() { return connected; }
private:
    bool connected;

    void send_item(const char *command, int i)
    {
        ostringstream osstr;
        osstr << command << " " << i << " " << Ops::get_item(i);
        write_command(osstr.str());
    }
};

#endif
