#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>

#include <sstream>
#include <iostream>

#include "clientstubbase.h"
#include "appname.h"
#include "immsutil.h"

using std::ostringstream;
using std::cerr;
using std::endl;

const std::string AppName = CLIENT_APP;

void IMMSClientStub::setup(bool use_xidle)
{
    ostringstream osstr;
    osstr << "Setup " << use_xidle;
    write_command(osstr.str());
}
void IMMSClientStub::start_song(int position, std::string path)
{
    ostringstream osstr;
    osstr << "StartSong " << position << " " << path;
    write_command(osstr.str());
}
void IMMSClientStub::end_song(bool at_the_end, bool jumped, bool bad)
{
    ostringstream osstr;
    osstr << "EndSong " << at_the_end << " " << jumped << " " << bad;
    write_command(osstr.str());
}
void IMMSClientStub::select_next() { write_command("SelectNext"); }
void IMMSClientStub::playlist_changed(int length)
{
#ifdef DEBUG
    LOG(ERROR) << "sending out pl len = " << length << endl;
#endif

    ostringstream osstr;
    osstr << "PlaylistChanged " << length;
    write_command(osstr.str());
}

int socket_connect(const string &sockname)
{
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un sun;
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, sockname.c_str(), sizeof(sun.sun_path));
    if (connect(fd, (sockaddr*)&sun, sizeof(sun)))
    {
        close(fd);
        LOG(ERROR) << "connection failed: " << strerror(errno) << endl;
        return -1;
    }
    return fd;
}
