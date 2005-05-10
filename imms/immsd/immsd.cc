#include <glib.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <errno.h>

#include "imms.h"
#include "strmanip.h"
#include "utils.h"
#include "serverstub.h"
#include "socketserver.h"

#define INTERFACE_VERSION "2.0"

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
    void check_playlist_item(int pos, const string &path)
    {
        string oldpath = imms->get_item_from_playlist(pos);
        if (oldpath != "")
        {
            if (oldpath != path)
            {
                cerr << "IMMSd: playlist triggered refresh: "
                    << oldpath << " != " << path << endl;
                write_command("PlaylistChanged");
            }
        }
        else
            imms->playlist_insert_item(pos, path);
    }
    void process_line(const string &line)
    {
        stringstream sstr;
        sstr << line;

        string command = "";
        sstr >> command;
#if defined(DEBUG) && 1
        if (command != "Playlist")
            std::cout << "> " << line << endl;
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
            sstr >> pos;
            string path;
            getline(sstr, path);
            path = path_normalize(path);
            check_playlist_item(pos, path);
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
            sstr >> pos;
            string path;
            getline(sstr, path);
            path = path_normalize(path);
            check_playlist_item(pos, path);
            return;
        }
        if (command == "Playlist")
        {
            int pos;
            sstr >> pos;
            string path;
            getline(sstr, path);
            path = path_normalize(path);
            imms->playlist_insert_item(pos, path);
            return;
        }
        if (command == "PlaylistEnd")
        {
            imms->playlist_ready();
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
            write_command("GetEntirePlaylist");

            return;
        }
        if (command == "SelectNext")
        {
            int pos = imms->select_next();
            if (pos == -1)
            {
                write_command("TryAgain");
                return;
            }
            write_command("EnqueueNext " + itos(pos));
            return;
        }
        if (command == "Version")
        {
            write_command("Version " INTERFACE_VERSION);
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
    int r = mkdir(get_imms_root().c_str(), 0700);
    if (r < 0 && errno != EEXIST)
    {
        cerr << "IMMSd: could not create directory " 
            << get_imms_root() << ": " << strerror(errno) << endl;
        exit(r);
    }

    StackLockFile lock(get_imms_root(".immsd_lock"));
    if (!lock.isok())
    {
        cerr << "IMMSd: another instance already active - exiting." << endl;
        exit(1);
    }

    // clean up after XMMS
    for (int i = 3; i < 255; ++i)
        close(i);

    loop = g_main_loop_new(NULL, FALSE);

    signal(SIGINT,  quit);
    signal(SIGTERM, quit);
    signal(SIGPIPE, SIG_IGN);

    GSource* ts = g_timeout_source_new(500);
    g_source_attach(ts, NULL);
    g_source_set_callback(ts, (GSourceFunc)do_events, NULL, NULL);

    ImmsDFilter filter(get_imms_root("socket"));

    cout << "IMMSd version " << PACKAGE_VERSION << " ready..." << endl;

    g_main_loop_run(loop);
    return 0;
}