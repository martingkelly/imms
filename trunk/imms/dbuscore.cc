#include "dbuscore.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>


IDBusMessageBase::IDBusMessageBase() : iter(new DBusMessageIter) {}

IDBusMessageBase::IDBusMessageBase(DBusMessage *message_)
    : message(message_), iter(new DBusMessageIter) {}

IDBusMessageBase::~IDBusMessageBase()
{
    delete iter;
    if (message)
        dbus_message_unref(message);
}

IDBusOMessage::IDBusOMessage(const string &path, const string &interface,
        const string &method, bool reply) 
{
    message = reply ?
            dbus_message_new_method_call(NULL, path.c_str(),
                    interface.c_str(), method.c_str()) : 
            dbus_message_new_signal(path.c_str(),
                    interface.c_str(), method.c_str());

    if (!message)
        throw IDBusException("Failed to create signal!");
    
    dbus_message_append_iter_init(message, iter);
}

IDBusOMessage::IDBusOMessage(DBusMessage *message_) : IDBusMessageBase(message_)
{
    dbus_message_append_iter_init(message, iter);
}

IDBusIMessage::IDBusIMessage(DBusMessage *message_) : IDBusMessageBase(message_)
{
    dbus_message_iter_init(message, iter);
    last = dbus_message_iter_has_next(iter);
}

void IDBusIMessage::verify_type(int type)
{
    if (last && ++last > 2)
        throw IDBusException("Read past the end of the iterator");
    if (dbus_message_iter_get_arg_type(iter) != type)
        throw IDBusException("Type mismatch!");
}

void IDBusIMessage::next()
{
    dbus_message_iter_next(iter);
    last = dbus_message_iter_has_next(iter);
}

DBusMessage* IDBusIMessage::reply()
{
    if (get_type() != MTMethod)
        throw IDBusException("Cannot reply to non-method call");
    DBusMessage *ret = dbus_message_new_method_return(message);
    if (!ret)
        throw IDBusException("Could not create reply message");
    return ret;
}

IDBusMessageType IDBusIMessage::get_type()
{
    switch (dbus_message_get_type(message))
    {
        case DBUS_MESSAGE_TYPE_SIGNAL:
            return MTSignal;
        case DBUS_MESSAGE_TYPE_METHOD_CALL:
            return MTMethod;
        case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            return MTReturn;
        case DBUS_MESSAGE_TYPE_ERROR:
            return MTError;
        default:
            return MTInvalid;
    };
}

string IDBusIMessage::get_interface()
{
    return dbus_message_get_interface(message); 
}

string IDBusIMessage::get_path()
{
    return dbus_message_get_path(message); 
}

string IDBusIMessage::get_member()
{
    if (get_type() != MTSignal && get_type() != MTMethod)
        throw IDBusException("Cannot get_member on this type!");
    return dbus_message_get_member(message); 
}

string IDBusIMessage::get_error()
{
    return dbus_message_get_error_name(message);
}

IDBusIMessage& IDBusIMessage::operator>>(int i)
{
    verify_type(DBUS_TYPE_INT32);
    i = dbus_message_iter_get_int32(iter);
    next();
    return *this;
}

IDBusIMessage& IDBusIMessage::operator>>(string &s)
{
    verify_type(DBUS_TYPE_STRING);
    s = dbus_message_iter_get_string(iter);
    next();
    return *this;
}

IDBusOMessage& IDBusOMessage::operator<<(int i)
{
    dbus_message_iter_append_int32(iter, i);
    return *this;
}

IDBusOMessage& IDBusOMessage::operator<<(const string &s)
{
    dbus_message_iter_append_string(iter, s.c_str());
    return *this;
} 

static DBusHandlerResult dispatch_wrapper(DBusConnection *con,
        DBusMessage *message, void *userdata)
{
    IDBusFilter *filter = (IDBusFilter*)userdata;
    IDBusConnection icon(con);
    dbus_message_ref(message);
    IDBusIMessage imessage(message);
    return filter->dispatch(icon, imessage) ?
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

IDBusClient::IDBusClient(const string &path, IDBusFilter *filter)
{
    DBusError error;
    dbus_error_init(&error);

    con = dbus_connection_open(("unix:abstract=" + path).c_str(), &error);
    if (!con)
    {
        string msg = "Failed to connect to " + path + ": " + error.message;
        dbus_error_free(&error);
        throw IDBusException(msg);
    }

    dbus_connection_setup_with_g_main(con, NULL);
    if (filter)
        dbus_connection_add_filter(con, dispatch_wrapper, filter, NULL);
}

void IDBusConnection::send(IDBusOMessage &message)
{
    dbus_message_iter_append_nil(message.iter);

    if (!dbus_connection_send(con, message.message, NULL))
        throw IDBusException("dbus_connection_send failed!");

    dbus_message_unref(message.message);
    message.message = 0;
    delete message.iter;
    message.iter = 0;
}

DBusMessage *IDBusConnection::send_with_reply(IDBusOMessage &message,
        int timeout)
{
    DBusError error;
    dbus_error_init(&error);

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(con,
         message.message, timeout, &error);

    if (dbus_error_is_set(&error))
    {
        string msg = "Failed send with reply: " + string(error.message);
        dbus_error_free(&error);
        throw IDBusException(msg);
    }

    if (!reply)
        throw IDBusException("Failed send with reply!");

    return reply;
}