#ifndef __PLAYER_H
#define __PLAYER_H

#include <string>

struct Player
{
    static int get_playlist_length();
    static int get_playlist_position();
    static std::string get_playlist_item(int index);
};

#endif
