#ifndef __PLAYER_H
#define __PLAYER_H

#include <string>

#include "dbuscore.h"

using std::string;

class IDBusPlayerControl
{
public:
    IDBusPlayerControl(IDBusConnection con_) : con(con_) {};
    int get_playlist_length()
    {
        IDBusOMessage m(IMMSCLIENTDBUSID, "GetPlaylistLength", true);
        IDBusIMessage reply(con.send_with_reply(m, 5000));
        int r;
        reply >> r;
        return r;
    } 
    string get_playlist_item(int index)
    {
        IDBusOMessage m(IMMSCLIENTDBUSID, "GetPlaylistItem", true);
        m << index;
        IDBusIMessage reply(con.send_with_reply(m, 5000));
        string r;
        reply >> r;
        return r;
    } 
    void reset_selection()
    {
        IDBusOMessage m(IMMSCLIENTDBUSID, "ResetSelection");
        con.send(m);
    } 
private:
    IDBusConnection con;
};

#endif
