#include "player.h"

extern int pl_length;
extern std::string imms_get_playlist_item(int at);

int Player::get_playlist_length() { return pl_length; }
std::string Player::get_playlist_item(int at)
                { return imms_get_playlist_item(at); }
