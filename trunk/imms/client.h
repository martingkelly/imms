#ifndef __CLIENT_H_
#define __CLIENT_H_

#include "dbuscore.h"
#include "utils.h"

#include "players/dbusclient.h"

template <typename Filter>
class IMMSClient
{
public:
    IMMSClient() : client(get_imms_root("socket"), &filter) {}
    void setup(bool use_xidle)
    {
        IDBusOMessage m(IMMSDBUSID, "Setup");
        m << use_xidle;
        client.send(m);
    }
    void start_song(int position, std::string path)
    {
        IDBusOMessage m(IMMSDBUSID, "StartSong");
        m << position;
        m << path;
        client.send(m);
    }
    void end_song(bool at_the_end, bool jumped, bool bad)
    {
        IDBusOMessage m(IMMSDBUSID, "EndSong");
        m << at_the_end;
        m << jumped;
        m << bad;
        client.send(m);
    }
    void select_next()
    {
        IDBusOMessage m(IMMSDBUSID, "SelectNext");
        client.send(m);
    }
    void playlist_changed(int length)
    {
        IDBusOMessage m(IMMSDBUSID, "PlaylistChanged");
        m << length;
        cerr << "sending out pl len = " << length << endl;
        client.send(m);
    }
private:
    Filter filter;
    IDBusClient client;
};

#endif
