#include "dbuscore.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>


IDBusMessageBase::IDBusMessageBase() : iter(new DBusMessageIter) {}

IDBusMessageBase::~IDBusMessageBase() { delete iter; }

IDBusMessage::IDBusMessage(const string &service, const string &interface,
        const string &method) : last(0)
{
    message = dbus_message_new_method_call(service.c_str(), "",
            interface.c_str(), method.c_str());

    if (!message)
        throw IDBusException("Failed to create signal!");
    
    dbus_message_append_iter_init(message, iter);
}

IDBusMessage::IDBusMessage(const string &interface,
        const string &method) : last(0)
{
    message = dbus_message_new_signal("", interface.c_str(), method.c_str());

    if (!message)
        throw IDBusException("Failed to create signal!");
    
    dbus_message_append_iter_init(message, iter);
}

IDBusMessage::IDBusMessage(DBusMessage *message_) : IDBusMessageBase(message_)
{
    last = dbus_message_iter_has_next(iter);
    dbus_message_iter_init(message, iter);
}

void IDBusMessage::verify_type(int type)
{
    if (last && ++last > 2)
        throw IDBusException("Read past the end of the iterator");
    if (dbus_message_iter_get_arg_type(iter) != type)
        throw IDBusException("Type mismatch!");
}

void IDBusMessage::next()
{
    dbus_message_iter_next(iter);
    last = dbus_message_iter_has_next(iter);
}

string IDBusMessage::get_interface()
{
    return dbus_message_get_interface(message); 
}

string IDBusMessage::get_path()
{
    return dbus_message_get_path(message); 
}

IDBusMessage& IDBusMessage::operator>>(int i)
{
    verify_type(DBUS_TYPE_INT32);
    i = dbus_message_iter_get_int32(iter);
    next();
    return *this;
}

IDBusMessage& IDBusMessage::operator>>(string &s)
{
    verify_type(DBUS_TYPE_STRING);
    s = dbus_message_iter_get_string(iter);
    next();
    return *this;
}

IDBusMessage& IDBusMessage::operator<<(int i)
{
    dbus_message_iter_append_int32(iter, i);
    return *this;
}

IDBusMessage& IDBusMessage::operator<<(const string &s)
{
    dbus_message_iter_append_string(iter, s.c_str());
    return *this;
} 

static DBusHandlerResult dispatch_wrapper(DBusConnection *con,
        DBusMessage *message, void *userdata)
{
    IDBusFilter *filter = (IDBusFilter*)userdata;
    return filter->dispatch(con, message) ?
        DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void new_connection_callback(DBusServer *server,
        DBusConnection *new_connection, void *userdata)
{
    dbus_connection_ref(new_connection);
    dbus_connection_setup_with_g_main(new_connection, NULL);

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

IDBusClient::IDBusClient(const string &path)
{
    DBusError error;
    dbus_error_init (&error);

    con = dbus_connection_open(("unix:abstract=" + path).c_str(), &error);
    if (!con)
    {
        string msg = "Failed to connect to " + path + ": " + error.message;
        dbus_error_free(&error);
        throw IDBusException(msg);
    }

    dbus_connection_setup_with_g_main(con, NULL);
}
