#ifndef __CLIENTSTUB_H_
#define __CLIENTSTUB_H_

#include "dbuscore.h"
#include "utils.h"
#include "dbusclient.h"

template <typename Filter>
class IMMSClient
{
public:
    IMMSClient() : filter(this), client(get_imms_root("socket"), &filter) {}

    void setup(bool use_xidle)
    {
        if (!client.isok())
            return;

        IDBusOMessage m(IMMSDBUSID, "Setup");
        m << use_xidle;
        client.send(m);
    }
    void start_song(int position, std::string path)
    {
        if (!client.isok())
            return;

        IDBusOMessage m(IMMSDBUSID, "StartSong");
        m << position;
        m << path;
        client.send(m);
    }
    void end_song(bool at_the_end, bool jumped, bool bad)
    {
        if (!client.isok())
            return;

        IDBusOMessage m(IMMSDBUSID, "EndSong");
        m << at_the_end;
        m << jumped;
        m << bad;
        client.send(m);
    }
    bool select_next()
    {
        if (!client.isok())
            return false;

        IDBusOMessage m(IMMSDBUSID, "SelectNext");
        client.send(m);
        return true;
    }
    void playlist_changed(int length)
    {
        if (!client.isok())
            return;

        IDBusOMessage m(IMMSDBUSID, "PlaylistChanged");
        m << length;
#ifdef DEBUG
        cerr << "sending out pl len = " << length << endl;
#endif
        client.send(m);
    }

    bool check_connection()
    {
        if (!client.isok())
        {
            client.connect();
            return client.isok();
        }
        return false;
    }

    void connection_lost() { client.connection_lost(); }

private:
    Filter filter;
    IDBusClient client;
};

template <typename Ops>
class ClientFilter : public IDBusFilter
{
    typedef IMMSClient< ClientFilter<Ops> > MyClient;
public:
    ClientFilter(MyClient *client) : client(client) {}
    virtual bool dispatch(IDBusConnection &con, IDBusIMessage &message)
    {
        if (message.get_type() == MTError)
        {
            cerr << "Error: " << message.get_error() << endl;
            return true;
        }
        if (message.get_type() == MTSignal)
        {
            if (message.get_interface() == "org.freedesktop.Local")
            {
                if (message.get_member() == "Disconnected")
                {
                    client->connection_lost();
                    return true;
                }
            }
            if (message.get_interface() != "org.luminal.IMMSClient")
            {
                cerr << "Received message on unknown interface: "
                    << message.get_interface() << endl;
                return false;
            }
            if (message.get_member() == "ResetSelection")
            {
                Ops::reset_selection();
                return true;
            }
            if (message.get_member() == "EnqueueNext")
            {
                int next;
                message >> next;
                Ops::set_next(next);
                return true;
            }
            if (message.get_member() == "RequestPlaylistChange")
            {
                client->playlist_changed(Ops::get_length());
                return true;
            }
            if (message.get_member() == "GetPlaylistItem")
            {
                int i;
                message >> i;
                send_item(con, i);
                return true;
            }
            if (message.get_member() == "GetEntirePlaylist")
            {
                for (int i = 0; i < Ops::get_length(); ++i)
                    send_item(con, i);
                return true;
            }

            g_print("Received signal %s %s\n",
                    message.get_interface().c_str(),
                    message.get_member().c_str());
        }
        else if (message.get_type() == MTMethod)
        {
            g_print("Received method call %s %s\n",
                    message.get_interface().c_str(),
                    message.get_member().c_str());

        }

        cerr << "Unhandled message!" << endl;

        return false;
    }
private:
    void send_item(IDBusConnection &con, int i)
    {
        IDBusOMessage m(IMMSDBUSID, "PlaylistItem");
        m << i << Ops::get_item(i);
        con.send(m);
    }

    MyClient *client;
};

#endif
