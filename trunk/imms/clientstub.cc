#include "clientstub.h"

#include <iostream>

using std::cerr;
using std::endl;

int IMMSClient::get_playlist_length()
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

string IMMSClient::get_playlist_item(int index)
{
    string path = "";
    try {
        IDBusOMessage m(IMMSCLIENTDBUSID, "GetPlaylistItem", true);
        m << index;
        IDBusIMessage reply(con.send_with_reply(m, 5000));
        reply >> path;
    } catch (IDBusException &e) {
        cerr << "Error: " << e.what() << endl;
    }
    return path;
}

void IMMSClient::reset_selection()
{
    try {
        IDBusOMessage m(IMMSCLIENTDBUSID, "ResetSelection");
        con.send(m);
    } catch (IDBusException &e) {
        cerr << "Error: " << e.what() << endl;
    }
}
