#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

void setup_connection_with_main_loop(DBusConnection *con, void *userdata)
{
    dbus_connection_setup_with_g_main(con, (GMainContext*)userdata);
}
