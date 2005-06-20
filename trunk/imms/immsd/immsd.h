#ifndef __IMMSD_H
#define __IMMSD_H

#include <string>

#include "imms.h"
#include "socketserver.h"

using std::string;

class SocketConnection : public GIOSocket
{
public:
    SocketConnection(int fd) : processor(0) { init(fd); }
    ~SocketConnection() { delete processor; }
    virtual void process_line(const string &line);
    virtual void connection_lost() { delete this; }
protected:
    LineProcessor *processor;
};

class ImmsProcessor : public IMMSServer, public LineProcessor
{
public:
    ImmsProcessor(SocketConnection *connection);
    ~ImmsProcessor();
    void write_command(const string &command)
        { connection->write(command + "\n"); }
    void check_playlist_item(int pos, const string &path);
    void process_line(const string &line);
protected:
    SocketConnection *connection;
};

#endif
