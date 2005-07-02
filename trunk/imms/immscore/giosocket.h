#ifndef __GIOSOCKET_H
#define __GIOSOCKET_H

#include <glib.h>

#include <string>
#include <list>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "immsconf.h"

using std::string;

class LineProcessor
{
public:
    virtual void process_line(const string &line) = 0;
    virtual ~LineProcessor() {}
};

class GIOSocket : public LineProcessor
{
public:
    GIOSocket() : con(0), read_tag(0), write_tag(0), outp(0) {}
    virtual ~GIOSocket() { close(); }

    bool isok() { return con; }

    void init(int fd)
    {
        fcntl(fd, F_SETFD, O_NONBLOCK);

        con = g_io_channel_unix_new(fd);
        read_tag = g_io_add_watch(con,
                (GIOCondition)(G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP),
                _read_event, this);
    }

    void write(const string &line)
    {
        if (outbuf.empty())
            write_tag = g_io_add_watch(con, G_IO_OUT, _write_event, this);

        outbuf.push_back(line);
    }

    void close()
    {
        if (con)
        {
            g_io_channel_close(con);
            g_io_channel_unref(con);
        }
        if (write_tag)
            g_source_remove(write_tag);
        if (read_tag)
            g_source_remove(read_tag);
        write_tag = read_tag = 0;
        inbuf = "";
        outbuf.clear();
        outp = 0;
        con = 0;
    }

    virtual void connection_lost() = 0;

    static gboolean _read_event(GIOChannel *source,
            GIOCondition condition, gpointer data)
    {
        GIOSocket *s = (GIOSocket*)data;
        return s->read_event(condition);
    }

    static gboolean _write_event(GIOChannel *source,
            GIOCondition condition, gpointer data)
    {
        GIOSocket *s = (GIOSocket*)data;
        return s->write_event(condition);
    }

    bool write_event(GIOCondition condition)
    {
        if (!con)
            return false;

        assert(condition & G_IO_OUT);

        if (!outp && !outbuf.empty())
            outp = outbuf.front().c_str();

        if (!outp)
            return (write_tag = 0);

        unsigned len = strlen(outp);
        gsize n = 0;
        GIOError e = g_io_channel_write(con, (char*)outp, len, &n);
        if (e == G_IO_ERROR_NONE)
        {
            if (n == len)
            {
                outbuf.pop_front();
                outp = 0;
                return outbuf.empty() ? (write_tag = 0) : 1;
            }
            outp += n;
        }

        return true;
    }

    bool read_event(GIOCondition condition)
    {
        if (!con)
            return false;

        if (condition & G_IO_HUP)
        {
            close();
            connection_lost();
#ifdef DEBUG
            std::cerr << "Connection terminated." << std::endl;
#endif
            return false;
        }

        if (condition & G_IO_IN)
        {
            gsize n = 0;
            GIOError e = g_io_channel_read(con, buf, sizeof(buf) - 1, &n);
            if (e == G_IO_ERROR_NONE)
            {
                buf[n] = '\0';
                char *lineend, *cur = buf;
                while ((lineend = strchr(cur, '\n')))
                {
                    *lineend = '\0';
                    inbuf += cur;
                    cur = lineend + 1;
                    process_line(inbuf);
                    inbuf = "";
                }
                inbuf += cur;
            }
        }

        return true;
    }

private:
    char buf[128];

    GIOChannel *con;
    int read_tag, write_tag;
    string inbuf;
    const char *outp;
    std::list<string> outbuf;
};

#endif
