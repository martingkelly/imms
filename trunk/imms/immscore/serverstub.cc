#include "serverstub.h"

#include <iostream>
#include <sstream>

using std::cerr;
using std::endl;
using std::ostringstream;

void IMMSServer::request_playlist_change()
{
    write_command("RequestPlaylistChange");
} 

void IMMSServer::request_playlist_item(int index)
{
    ostringstream osstr;
    osstr << "GetPlaylistItem " << index;
    write_command(osstr.str());
}

void IMMSServer::request_entire_playlist()
{
    write_command("GetEntirePlaylist");
}

void IMMSServer::reset_selection()
{
    write_command("ResetSelection");
}
