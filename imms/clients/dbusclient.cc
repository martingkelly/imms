#include "dbusclient.h"
#include <dbus/dbus.h>

#include <iostream>
#include <unistd.h>

using std::cerr;
using std::endl;

DBusHandlerResult dispatch_wrapper(DBusConnection *con,
        DBusMessage *message, void *userdata);

IDBusClient::IDBusClient(const string &path, IDBusFilter *filter)
    : path(path), filter(filter)
{
    connect();
}

IDBusClient::~IDBusClient()
{
    connection_lost();
}

void IDBusClient::connection_lost()
{
    if (con)
    {
        cerr << "IMMS: Connection to server lost!" << endl;
        dbus_connection_disconnect(con);
        dbus_connection_unref(con);
    }
    con = 0;
}

void IDBusClient::connect()
{
    system("immsd &");

    DBusError error;
    dbus_error_init(&error);

    con = dbus_connection_open(("unix:abstract=" + path).c_str(), &error);
    if (!con)
    {
        cerr << "Failed to connect to " << path
            << ": " << error.message << endl;
        dbus_error_free(&error);
        return;
    }

    setup_connection_with_main_loop(con, 0);
    if (filter)
        dbus_connection_add_filter(con, dispatch_wrapper, filter, NULL);
}
