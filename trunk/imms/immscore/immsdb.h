#ifndef __IMMSDB_H
#define __IMMSDB_H

#include "basicdb.h"
#include "playlist.h"
#include "correlate.h"

#define SCHEMA_VERSION 13

class ImmsDb : virtual public BasicDb,
                       public PlaylistDb,
                       public CorrelationDb
{
public:
    ImmsDb();
protected:
    virtual void sql_create_tables();
    virtual void sql_schema_upgrade(int from = 0);
};

#endif
