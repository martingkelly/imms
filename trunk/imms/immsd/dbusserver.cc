#include "dbusserver.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

DBusHandlerResult dispatch_wrapper(DBusConnection *con,
        DBusMessage *message, void *userdata);

static void new_connection_callback(DBusServer *server,
        DBusConnection *new_connection, void *userdata)
{
    dbus_connection_ref(new_connection);
    dbus_connection_setup_with_g_main(new_connection, NULL);

    IDBusConnection con(new_connection);
    reinterpret_cast<IDBusFilter*>(userdata)->new_connection(con);

    dbus_connection_add_filter(new_connection, dispatch_wrapper,
            userdata, NULL);
}

IDBusServer::IDBusServer(const string &path, IDBusFilter *filter)
{
    DBusError error;
    dbus_error_init(&error);

    server = dbus_server_listen(("unix:abstract=" + path).c_str(), &error);
    if (!server)
    {
        string errormsg = error.message;
        dbus_error_free(&error);
        throw IDBusException("Could not create a socked at "
                + path  + ": " + errormsg);
    }

    dbus_server_set_new_connection_function(server,
            new_connection_callback, filter, NULL);

    dbus_server_setup_with_g_main(server, NULL);
}
