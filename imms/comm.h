#ifndef __COMM_H
#define __COMM_H

#include <string>

using std::string;

class Socket
{
public:
    Socket(int _fd = -1);
    Socket &operator <<(const string &msg) { write(msg); return *this; }
    void write(const string &msg);
    string read();
    bool isok() { return fd > 0; }
    void close();
    int getfd() { return fd; }
    ~Socket();
protected:
    int fd;
};

class SocketClient : public Socket
{
public:
    SocketClient(const string &path);
};

class SocketServer : public Socket
{
public:
    SocketServer(const string &path);
    int accept();
};

#endif
