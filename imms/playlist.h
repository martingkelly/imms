#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include "immsdb.h"

class PlaylistDB : public ImmsDb
{
public:
    void playlist_insert_item(int pos, const string &path);
    void playlist_update_identity(int pos);
    void playlist_mass_identify();
    bool playlist_id_from_item(int pos);

    string get_playlist_item(int pos);
    int get_unknown_playlist_item();

    void clear_playlist();
protected:
    virtual void sql_create_tables();
private:
    bool all_known;
};

#endif
