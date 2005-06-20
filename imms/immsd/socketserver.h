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
