#ifndef __GIOSOCKET_H
#define __GIOSOCKET_H

#include <glib.h>

#include <string>
#include <list>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include "immsconf.h"

using std::string;
using std::list;
using std::cerr;
using std::endl;

class GIOSocket
{
public:
    GIOSocket() : con(0), outp(0) {}
    virtual ~GIOSocket() { close(); }

    bool isok() { return con; }
    void init(int fd)
    {
        fcntl(fd, F_SETFD, O_NONBLOCK);

        con = g_io_channel_unix_new(fd);
        g_io_add_watch(con,
                (GIOCondition)(G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP),
                _connection_event, this);
    }
    void write(const string &line)
    {
        if (outbuf.empty())
            g_io_add_watch(con, G_IO_OUT, _connection_event, this);

        outbuf.push_back(line);
    }
    void close()
    {
        if (con)
        {
            g_io_channel_close(con);
            g_io_channel_unref(con);
        }
        inbuf = "";
        outbuf.clear();
        outp = 0;
        con = 0;
    }

    virtual void process_line(const string &line) = 0;
    virtual void connection_lost() = 0;

    static gboolean _connection_event(GIOChannel *source,
            GIOCondition condition, gpointer data)
    {
        GIOSocket *s = (GIOSocket*)data;
        return s->connection_event(condition);
    }

    bool connection_event(GIOCondition condition)
    {
        static char buf[128];

        if (!con)
            return false;

        if (condition & G_IO_IN)
        {
            unsigned n = 0;
            GIOError e = g_io_channel_read(con, buf, sizeof(buf), &n);
            if (e == G_IO_ERROR_NONE)
            {
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
                GIOError e = g_io_channel_write(con, (char*)outp, len, &n);
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
            close();
            cerr << "Connection terminated." << endl;
            return false;
        }

        return true;
    }

private:
    GIOChannel *con;
    string inbuf;
    const char *outp;
    std::list<string> outbuf;
};

#endif
