#ifndef __DBUSCLIENT_H
#define __DBUSCLIENT_H

#include "dbuscore.h"

void setup_connection_with_main_loop(DBusConnection *con, void *userdata);

class IDBusClient : public IDBusConnection
{
public:
    IDBusClient(const string &path, IDBusFilter *filter);
    ~IDBusClient();

    bool isok();
    void connection_lost();
    void connect();
private:
    string path;
    IDBusFilter *filter;
};

#endif
