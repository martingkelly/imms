#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include "immsconf.h"
#include "basicdb.h"

#include <vector>

class PlaylistDb : virtual public BasicDb
{
public:
    void playlist_insert_item(int pos, const string &path);
    void playlist_update_identity(int pos);
    void playlist_mass_identify();
    bool playlist_id_from_item(int pos);

    string get_playlist_item(int pos);
    int get_unknown_playlist_item();

    int install_filter(const string &filter);
    int random_playlist_position();

    void playlist_clear();
protected:
    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0) {};

private:
    int filtercount;
};

#endif
