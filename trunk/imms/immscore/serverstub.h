#ifndef __SERVERSTUB_H
#define __SERVERSTUB_H

#include <string>

#include "dbuscore.h"

using std::string;

class IMMSServer
{
public:
    IMMSServer(IDBusConnection con_) : conready(false), con(con_) {};
    int get_playlist_length();
    string get_playlist_item(int index);
    void reset_selection();
    void ready();
protected:
    bool conready;
private:
    IDBusConnection con;
};

#endif
