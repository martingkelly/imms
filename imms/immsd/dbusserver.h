#ifndef __DBUSSERVER_H
#define __DBUSSERVER_H

#include "dbuscore.h"

class IDBusServer
{
public:
    IDBusServer(const string &path, IDBusFilter *filter);
private:
    DBusServer *server;
};

#endif
