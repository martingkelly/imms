#ifndef __CLIENTSTUB_H
#define __CLIENTSTUB_H

#include <string>

#include "dbuscore.h"

using std::string;

class IMMSClient
{
public:
    IMMSClient(IDBusConnection con_) : con(con_) {};
    int get_playlist_length();
    string get_playlist_item(int index);
    void reset_selection();
private:
    IDBusConnection con;
};

#endif
