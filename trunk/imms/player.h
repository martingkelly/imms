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
        int r = 0;
        try {
            IDBusOMessage m(IMMSCLIENTDBUSID, "GetPlaylistLength", true);
            IDBusIMessage reply(con.send_with_reply(m, 5000));
            reply >> r;
        } catch (IDBusException &e) {
            cerr << "Error: " << e.what() << endl;
        }
        return r;
    } 
    string get_playlist_item(int index)
    {
        string path = "";
        try {
            IDBusOMessage m(IMMSCLIENTDBUSID, "GetPlaylistItem", true);
            m << index;
            IDBusIMessage reply(con.send_with_reply(m, 10000));
            reply >> path;
        } catch (IDBusException &e) {
            cerr << "Error: " << e.what() << endl;
        }
        return path;
    } 
    void reset_selection()
    {
        try {
            IDBusOMessage m(IMMSCLIENTDBUSID, "ResetSelection");
            con.send(m);
        } catch (IDBusException &e) {
            cerr << "Error: " << e.what() << endl;
        }
    } 
private:
    IDBusConnection con;
};

#endif
