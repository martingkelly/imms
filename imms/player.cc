#include "player.h"

extern int pl_length;
extern int cur_plpos;
extern std::string imms_get_playlist_item(int at);

int Player::get_playlist_length() { return pl_length; }
int Player::get_playlist_position() { return cur_plpos; }
std::string Player::get_playlist_item(int at)
                { return imms_get_playlist_item(at); }
