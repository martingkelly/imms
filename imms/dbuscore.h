#ifndef __IMMSDBUS_H
#define __IMMSDBUS_H

#include <string>

#define IMMSDBUSID "/org/luminal/IMMS", "org.luminal.IMMS"
#define IMMSCLIENTDBUSID "/org/luminal/IMMSClient", "org.luminal.IMMSClient"

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
    friend class IDBusConnection;
public:
    IDBusMessageBase();
    IDBusMessageBase(DBusMessage *message);
    ~IDBusMessageBase();
protected:
    DBusMessage *message;
    DBusMessageIter *iter;
};

class IDBusOMessage : protected IDBusMessageBase
{
public:
    IDBusOMessage(const string &path, const string &interface,
            const string &method, bool reply = false);

    IDBusOMessage(DBusMessage *message);

    IDBusOMessage &operator<<(int i);
    IDBusOMessage &operator<<(const string &s);
};

enum IDBusMessageType
{
    MTInvalid,
    MTSignal,
    MTMethod,
    MTReturn,
    MTError
};

class IDBusIMessage : protected IDBusMessageBase
{
public:
    IDBusIMessage(DBusMessage *message_);

    IDBusIMessage &operator>>(int i);
    IDBusIMessage &operator>>(string &s);

    string get_interface();
    string get_path();
    string get_member();
    string get_error();

    IDBusMessageType get_type();

    DBusMessage *reply();

protected:
    void verify_type(int type);
    void next();

    int last;
};

class IDBusConnection
{
public:
    IDBusConnection(DBusConnection *con_ = 0) : con(con_) {}
    void send(IDBusOMessage &message);
    DBusMessage *send_with_reply(IDBusOMessage &message, int timeout);

    void store(int index, void *p);
    void *retrieve(int index);

    bool operator!=(const IDBusConnection &other)
        { return other.con != con; }

protected:
    DBusConnection *con;
};

class IDBusFilter
{
public:
    virtual void new_connection(IDBusConnection &con) {};
    virtual bool dispatch(IDBusConnection &con, IDBusIMessage &message) = 0;
};

class IDBusServer
{
public:
    IDBusServer(const string &path, IDBusFilter *filter);
private:
    DBusServer *server;
};

class IDBusClient : public IDBusConnection
{
public:
    IDBusClient(const string &path, IDBusFilter *filter);
};

#endif
