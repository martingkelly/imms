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
#ifndef __SOCKETSERVER_H
#define __SOCKETSERVER_H

#include <string>

#include "immsconf.h"
#include "giosocket.h"

using std::string;

class SocketListenerBase
{
public:
    SocketListenerBase(const string &sockpath);
    virtual ~SocketListenerBase();

    static gboolean incoming_connection_helper(GIOChannel *source,
            GIOCondition condition, gpointer data);

    virtual void incoming_connection(int fd) = 0;
protected:
    GIOChannel *listener;
};

template <typename Connection>
class SocketListener : public SocketListenerBase
{
public:
    SocketListener(const string &sockpath) : SocketListenerBase(sockpath) {};
    virtual void incoming_connection(int fd) { new Connection(fd); }
};

#endif
