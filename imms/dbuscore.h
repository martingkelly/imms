#ifndef __IMMSDBUS_H
#define __IMMSDBUS_H

#include <string>

using std::string;

class IDBusException : public std::exception
{
public:
    IDBusException(const string &error) : msg(error) {};
    const string &what() { return msg; }
    ~IDBusException() throw () {};
private:
    string msg;
};

struct DBusServer;
struct DBusMessage;
struct DBusConnection;
struct DBusMessageIter;

class IDBusMessageBase
{
public:
    IDBusMessageBase();
    IDBusMessageBase(DBusMessage *message_) : message(message_) {}
    ~IDBusMessageBase();
protected:
    DBusMessage *message;
    DBusMessageIter *iter;
};

class IDBusMessage : protected IDBusMessageBase
{
public:
    IDBusMessage(const string &service, const string &interface,
            const string &method);
    IDBusMessage(const string &interface, const string &method);
    IDBusMessage(DBusMessage *message_);

    IDBusMessage &operator>>(int i);
    IDBusMessage &operator>>(string &s);
    IDBusMessage &operator<<(int i);
    IDBusMessage &operator<<(const string &s);

    string get_interface();
    string get_path();

    void send();

protected:
    void verify_type(int type);
    void next();

    int last;
};

class IDBusFilter
{
public:
    virtual bool dispatch(DBusConnection *con, DBusMessage *message) = 0;
};

class IDBusServer
{
public:
    IDBusServer(const string &path, IDBusFilter *filter);
private:
    DBusServer *server;
};

class IDBusClient
{
public:
    IDBusClient(const string &path);
    DBusConnection *get_bus() { return con; }
private:
    DBusConnection *con;
};

#endif
