#include "socketserver.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#include <iostream>

using std::endl;
using std::cerr;

static gboolean incoming_connection(GIOChannel *source,
        GIOCondition condition, gpointer data)
{
    SocketServer *ss = (SocketServer*)data;
    ss->_connection_established();
    return true;
}

SocketServer::SocketServer(const string &sockpath)
{
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        cerr << "Could not create a socket: " << strerror(errno) << endl;
        return;
    }

    struct sockaddr_un sun;
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, sockpath.c_str(), sizeof(sun.sun_path));
    unlink(sockpath.c_str());
    if (bind(fd, (sockaddr*)&sun, sizeof(sun)))
    {
        cerr << "Could not bind socket: " << strerror(errno) << endl;
        return;
    }

    if (listen(fd, 5))
    {
        cerr << "Could not listen: " << strerror(errno) << endl;
        return;
    }

    listener = g_io_channel_unix_new(fd);
    g_io_add_watch(listener, (GIOCondition)(G_IO_IN | G_IO_PRI),
            ::incoming_connection, this);
}

SocketServer::~SocketServer()
{
    if (listener)
    {
        g_io_channel_close(listener);
        g_io_channel_unref(listener);
        listener = 0;
    }
}

void SocketServer::_connection_established()
{
    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(sun));
    socklen_t size = 0;
    int fd = accept(g_io_channel_unix_get_fd(listener),
            (sockaddr*)&sun, &size);

    init(fd);

#ifdef DEBUG
    cerr << "Incoming connection!" << endl;
#endif

    connection_established();
}
