#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>

#include "immsconf.h"
#include "comm.h"

using std::cerr;
using std::endl;

Socket::Socket(int _fd) : fd(_fd)
{
    if (fd > 0)
        fcntl(fd, F_SETFL, O_RDWR | O_NONBLOCK);
}

Socket::~Socket()
{
    close();
}

void Socket::close()
{
    if (isok())
    {
        ::close(fd);
        fd = -1;
    }
}

string Socket::read()
{
    if (!isok())
        return "";

    char buf[256];
    int r = ::read(fd, buf, sizeof(buf));

    if (r < 0)
    {
        if (errno == EAGAIN)
            return "";
        cerr << "read: error: " << strerror(errno) << endl;
        close();
    }

    return string(buf, 0, r);
}

void Socket::write(const string& data)
{
    if (!isok())
        return;

    int r = ::write(fd, data.c_str(), data.length());
    if (r != (int)data.length())
    {
        cerr << "write: error: " << strerror(errno) << endl;
        close();
    }
}

SocketClient::SocketClient(const string &path)
{
    struct sockaddr_un un;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd < 0)
        return;

    un.sun_family = AF_UNIX;
    strncpy(un.sun_path, path.c_str(), sizeof(un.sun_path));

    int r = connect(fd, (struct sockaddr *) &un,
            sizeof(struct sockaddr_un));

    fcntl(fd, F_SETFL, O_RDWR | O_NONBLOCK);

    if (r < 0)
    {
        cerr << "client: connect failed: " << strerror(errno) << endl;
        close();
    }
}

SocketServer::SocketServer(const string &path)
{
    unlink(path.c_str());

    struct sockaddr_un un;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd < 0)
        return;

    un.sun_family = AF_UNIX;

    strncpy(un.sun_path, path.c_str(), sizeof(un.sun_path));

    int r = bind(fd, (struct sockaddr *) &un,
            sizeof(struct sockaddr_un));

    if (r < 0)
    {
        close();
        return;
    }

    fcntl(fd, F_SETFL, O_RDWR | O_NONBLOCK);

    listen(fd, 2);
}               

int SocketServer::accept()
{
    return ::accept(fd, NULL, NULL);
}
