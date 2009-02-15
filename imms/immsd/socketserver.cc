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
#include "socketserver.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>

#include <cstdlib>
#include <iostream>

using std::endl;
using std::cerr;

SocketListenerBase::SocketListenerBase(const string &sockpath)
{
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        cerr << "Could not create a socket: " << strerror(errno) << endl;
        exit(3);
    }

    struct sockaddr_un sun;
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, sockpath.c_str(), sizeof(sun.sun_path));
    unlink(sockpath.c_str());
    if (bind(fd, (sockaddr*)&sun, sizeof(sun)))
    {
        cerr << "Could not bind socket: " << strerror(errno) << endl;
        exit(4);
    }

    if (listen(fd, 5))
    {
        cerr << "Could not listen: " << strerror(errno) << endl;
        exit(5);
    }

    listener = g_io_channel_unix_new(fd);
    g_io_add_watch(listener, (GIOCondition)(G_IO_IN | G_IO_PRI),
            incoming_connection_helper, this);
}

SocketListenerBase::~SocketListenerBase()
{
    if (listener)
    {
        g_io_channel_close(listener);
        g_io_channel_unref(listener);
        listener = 0;
    }
}

gboolean SocketListenerBase::incoming_connection_helper(
        GIOChannel *source, GIOCondition condition, gpointer data)
{
    SocketListenerBase *ss = (SocketListenerBase*)data;

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(sun));
    socklen_t size = 0;
    int fd = accept(g_io_channel_unix_get_fd(ss->listener),
            (sockaddr*)&sun, &size);

#ifdef DEBUG
    cerr << "Incoming connection!" << endl;
#endif

    if (fd != -1)
        ss->incoming_connection(fd);
    return true;
}
