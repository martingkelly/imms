#include <glib.h>

#include "imms.h"
#include "utils.h"
#include "dbuscore.h"
#include "dbusserver.h"

int pl_length;
int cur_plpos;

IDBusConnection activecon;
Imms *imms;

gboolean do_events(void *unused)
{
    if (imms)
        imms->do_events();
    return TRUE;
}

class ImmsDFilter : public IDBusFilter 
{
    void new_connection(IDBusConnection &con)
    {
        if (!imms)
        {
#ifdef DEBUG
            cerr << "New client connected!" << endl;
#endif
            activecon = con;
            imms = new Imms(con);
        }
    }
    bool dispatch(IDBusConnection &con, IDBusIMessage &message)
    {
        if (message.get_type() == MTError)
        {
            cerr << "Error: " << message.get_error() << endl;
            return true;
        }              

        if (!imms || activecon != con)
            return false;

        if (message.get_type() == MTSignal)
        {
            if (message.get_interface() == "org.freedesktop.Local")
            {
                if (message.get_member() == "Disconnected")
                {
#ifdef DEBUG
                    cerr << "Client diconnected!" << endl;
#endif
                    delete imms;
                    imms = 0;
                    return true;
                }
            }

            if (message.get_interface() != "org.luminal.IMMS")
            {
                cerr << "Recieved message " << message.get_member()
                    << " on unknown interface: "
                    << message.get_interface() << endl;
                return false;
            }
            if (message.get_member() == "Setup")
            {
                bool use_xidle;
                message >> use_xidle;
                imms->setup(use_xidle);
                return true;
            }
            if (message.get_member() == "StartSong")
            {
                int pos;
                string path;
                message >> pos >> path;
                imms->start_song(pos, path);
                return true;
            }
            if (message.get_member() == "EndSong")
            {
                bool end, jumped, bad;
                message >> end >> jumped >> bad;
                imms->end_song(end, jumped, bad);
                return true;
            }
            if (message.get_member() == "PlaylistChanged")
            {
                int length;
                message >> length;
                cerr << "got playlist length = " << length << endl;
                imms->playlist_changed(length);
                return true;
            }
            if (message.get_member() == "SelectNext")
            {
                IDBusOMessage m(IMMSCLIENTDBUSID, "EnqueueNext");
                m << imms->select_next();
                con.send(m);
                return true;
            }

            cerr << "Unhandled signal: " << message.get_member() << "!" << endl;
        }
        else if (message.get_type() == MTMethod)
        {
            cerr << "Unhandled method: " << message.get_member() << "!" << endl;
        }

        return false;
    }
};

int main(int argc, char **argv)
{
    StackLockFile lock(get_imms_root(".immsd_lock"));
    if (!lock.isok())
    {
        cerr << "IMMSd: another instance already active - exiting." << endl;
        exit(1);
    }

    GMainLoop *loop = g_main_loop_new (NULL, FALSE);

    ImmsDFilter filter;
    IDBusServer server(get_imms_root("socket"), &filter);

    GSource* ts = g_timeout_source_new(500);
    g_source_attach(ts, NULL);
    g_source_set_callback(ts, (GSourceFunc)do_events, NULL, NULL);

    g_main_loop_run(loop);
    return 0;
}
