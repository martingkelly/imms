#ifndef __SOCKETSERVER_H
#define __SOCKETSERVER_H

#include <glib.h>

#include <string>
#include <list>

#include "immsconf.h"

using std::string;

class SocketServer
{
public:
    SocketServer(const string &sockpath);
    virtual ~SocketServer();

    virtual void process_line(const string &line) = 0;
    virtual void connection_established() = 0;
    virtual void connection_lost() = 0;

    void write(const string &line);

    bool _connection_event(GIOCondition condition);
    void _connection_established();
protected:
    GIOChannel *listener, *con;

    string inbuf;
    const char *outp;
    std::list<string> outbuf;
};

#endif
