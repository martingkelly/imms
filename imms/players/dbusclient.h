#ifndef __DBUSCLIENT_H
#define __DBUSCLIENT_H

#include "dbuscore.h"
#include <glib.h>

void setup_connection_with_main_loop(DBusConnection *con, void *userdata);

class IDBusClient : public IDBusConnection
{
public:
    IDBusClient(const string &path, IDBusFilter *filter);
    ~IDBusClient();
};

#ifdef GLIB2
#include <dbus-glib.h>
void setup_connection_with_main_loop(DBusConnection *con, void *userdata)
{
    dbus_connection_setup_with_g_main(con, userdata);
}
#endif

#endif
