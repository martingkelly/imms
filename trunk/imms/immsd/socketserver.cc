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

static gboolean connection_event(GIOChannel *source,
        GIOCondition condition, gpointer data)
{
    SocketServer *ss = (SocketServer*)data;
    return ss->_connection_event(condition);
}

SocketServer::SocketServer(const string &sockpath) : con(0), outp(0)
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
    if (con)
    {
        cerr << "IMMS: Another session is already active" << endl;
        return;
    }

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(sun));
    socklen_t size = 0;
    int fd = accept(g_io_channel_unix_get_fd(listener),
            (sockaddr*)&sun, &size);
    con = g_io_channel_unix_new(fd);
    g_io_add_watch(con,
            (GIOCondition)(G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP),
            ::connection_event, this);

#ifdef DEBUG
    cerr << "Incoming connection!" << endl;
#endif

    connection_established();
}

void SocketServer::write(const string &line)
{
    if (outbuf.empty())
        g_io_add_watch(con, G_IO_OUT, ::connection_event, this);

    outbuf.push_back(line);
}

char buf[128];

bool SocketServer::_connection_event(GIOCondition condition)
{
    if (!con)
        return false;

    if (condition & G_IO_IN)
    {
        unsigned n = 0;
        GIOError e = g_io_channel_read(con, buf, sizeof(buf), &n);
        if (e == G_IO_ERROR_NONE)
        {
            cerr << "read " << n << " bytes" << endl;
            buf[n] = '\0';
            char *lineend = strchr(buf, '\n');
            if (lineend)
            {
                *lineend = '\0';
                inbuf += buf;
                process_line(inbuf);
                inbuf = lineend + 1;
            }
            else
                inbuf += buf;
        }       
        else 
            cerr << "some error occured" << endl;
    }

    if (condition & G_IO_OUT)
    {
        if (!outp && !outbuf.empty())
            outp = outbuf.front().c_str();

        if (outp)
        {
            unsigned len = strlen(outp);
            unsigned n = 0;
            GIOError e = g_io_channel_write(con, outp, len, &n);
            if (e == G_IO_ERROR_NONE)
            {
                if (n == len)
                {
                    outbuf.pop_front();
                    outp = 0;
                    return !outbuf.empty();
                }
                outp += n;
            }
            else
                cerr << "some error occured" << endl;
        }
    }

    if (condition & G_IO_HUP)
    {
        connection_lost();
        g_io_channel_close(con);
        g_io_channel_unref(con);
        con = 0;
        cerr << "Connection terminated." << endl;
        return false;
    }

    return true;
}
