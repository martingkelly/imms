#ifndef __CLIENTSTUB_H_
#define __CLIENTSTUB_H_

#include "giosocket.h"
#include "utils.h"
#include "clientstubbase.h"

#include <stdlib.h>

#include <sstream>
#include <iostream>

using std::stringstream;
using std::ostringstream;
using std::cerr;
using std::endl;

template <typename Ops>
class IMMSClient : public IMMSClientStub, protected GIOSocket 
{
public:
    IMMSClient() : connected(false) { }
    bool connect()
    {
        int fd = socket_connect(get_imms_root("socket"));
        if (fd > 0)
        {
            init(fd);
            return connected = true;
        }
        cerr << "Connection failed: " << strerror(errno) << endl;
        return false;
    }
    virtual void write_command(const string &line)
        { if (isok()) GIOSocket::write(line + "\n"); }
    virtual void process_line(const string &line)
    {
        stringstream sstr;
        sstr << line;

#if defined(DEBUG) && 0
        cerr << "< " << line << endl;
#endif

        string command = "";
        sstr >> command;

        if (command == "ResetSelection")
        {
            Ops::reset_selection();
            return;
        }
        if (command == "TryAgain")
        {
            write_command("SelectNext");
            return;
        }
        if (command == "EnqueueNext")
        {
            int next;
            sstr >> next;
            Ops::set_next(next);
            return;
        }
        if (command == "PlaylistChanged")
        {
            IMMSClientStub::playlist_changed(Ops::get_length());
            return;
        }
        if (command == "GetPlaylistItem")
        {
            int i;
            sstr >> i;
            send_item(i);
            return;
        }
        if (command == "GetEntirePlaylist")
        {
            for (int i = 0; i < Ops::get_length(); ++i)
                send_item(i);
            return;
        }

        cerr << "IMMS: Unknown command: " << command << endl;
    }
    virtual void connection_lost() { connected = false; }

    bool check_connection()
    {
        if (isok())
            return false;

        system("immsd &");

        return connect();
    }
    
    bool isok() { return connected; }
private:
    bool connected;

    void send_item(int i)
    {
        ostringstream osstr;
        osstr << "PlaylistItem " << i << " " << Ops::get_item(i);
        write_command(osstr.str());
    }
};

#endif
