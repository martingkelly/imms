#ifndef __IMMSBASE_H
#define __IMMSBASE_H

#include "playlist.h"

class ImmsBase
{
    class DirMaker
    {
    public:
        DirMaker();
    };
protected:
    DirMaker dirmaker;
    PlaylistDB immsdb;
};

#endif
