#include <glib.h>
#include <signal.h>
#include <iostream>
#include <sstream>

#include "imms.h"
#include "utils.h"
#include "serverstub.h"
#include "socketserver.h"

using std::cerr;
using std::cout;
using std::endl;
using std::stringstream;

Imms *imms;

gboolean do_events(void *unused)
{
    if (imms)
        imms->do_events();
    return TRUE;
}

class ImmsDFilter : public SocketServer, public IMMSServer
{
public:
    ImmsDFilter(const string &sockname) : SocketServer(sockname) {}
    void write_command(const string &command)
    {
        SocketServer::write(command + "\n");
    }
    void connection_established()
    {
        if (!imms)
            imms = new Imms(this);
    }
    void connection_lost()
    {
        delete imms;
        imms = 0;

        exit(0);
    }
    void process_line(const string &line)
    {
        stringstream sstr;
        sstr << line;

        string command = "";
        sstr >> command;
#ifdef DEBUG
        cerr << "Got line: " << line << endl;
        cerr << "The command is: " << command << endl;
#endif

        if (command == "Setup")
        {
            bool use_xidle;
            sstr >> use_xidle;
            imms->setup(use_xidle);
            return;
        }
        if (command == "StartSong")
        {
            int pos;
            string path;
            sstr >> pos >> path;
            imms->start_song(pos, path);
            return;
        }
        if (command == "EndSong")
        {
            bool end, jumped, bad;
            sstr >> end >> jumped >> bad;
            imms->end_song(end, jumped, bad);
            return;
        }
        if (command == "PlaylistItem")
        {
            int pos;
            string path;
            sstr >> pos >> path;
            string oldpath = imms->get_item_from_playlist(pos);
            cerr << "item " << pos << ": " << path << endl;
            if (oldpath != "")
            {
                if (oldpath != path)
                    write_command("PlaylistChanged");
            }
            else
                imms->playlist_insert_item(pos, path);
            return;
        }
        if (command == "PlaylistChanged")
        {
            int length;
            sstr >> length;
#ifdef DEBUG
            cerr << "got playlist length = " << length << endl;
#endif
            imms->playlist_changed(length);

            return;
        }
        if (command == "SelectNext")
        {
            return;
        }
        cerr << "IMMSd: Unknown command: " << command << endl;
    }
};

GMainLoop *loop = 0;

void quit(int signum)
{
    if (loop)
        g_main_quit(loop);
    loop = 0;
    signal(signum, SIG_DFL);
}

int main(int argc, char **argv)
{
    StackLockFile lock(get_imms_root(".immsd_lock"));
    if (!lock.isok())
    {
        cerr << "IMMSd: another instance already active - exiting." << endl;
        exit(1);
    }

    loop = g_main_loop_new(NULL, FALSE);

    signal(SIGINT,  quit);
    signal(SIGTERM, quit);

    GSource* ts = g_timeout_source_new(500);
    g_source_attach(ts, NULL);
    g_source_set_callback(ts, (GSourceFunc)do_events, NULL, NULL);

    ImmsDFilter filter(get_imms_root("socket"));

    cout << "IMMSd version " << PACKAGE_VERSION << " ready..." << endl;

    g_main_loop_run(loop);
    return 0;
}
