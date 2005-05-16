#ifndef __SOCKETSERVER_H
#define __SOCKETSERVER_H

#include <string>

#include "immsconf.h"
#include "giosocket.h"

using std::string;

class SocketServer : public GIOSocket
{
public:
    SocketServer(const string &sockpath);
    virtual ~SocketServer();

    virtual void connection_established() = 0;

    void _connection_established();
protected:
    GIOChannel *listener;
};

#endif
