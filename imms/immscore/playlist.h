#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include "immsconf.h"
#include "basicdb.h"
#include "song.h"

#include <vector>

class PlaylistDb
{
public:
    PlaylistDb() : filtercount(-1) {}
    void playlist_insert_item(int pos, const string &path);
    void playlist_update_identity(int pos, int uid);
    void playlist_mass_identify();
    static Song playlist_id_from_item(int pos);

    string get_item_from_playlist(int pos);
    int get_unknown_playlist_item();

    int install_filter(const string &filter);
    int random_playlist_position();
    int get_effective_playlist_length(bool nofilter = false);

    time_t get_average_first_seen();
    time_t get_average_last();

    void playlist_clear();

protected:
    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0) {};

private:
    int filtercount;
};

#endif
