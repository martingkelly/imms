#ifndef __IMMSBASE_H
#define __IMMSBASE_H

#include "immsdb.h"

class ImmsBase
{
    class DirMaker
    {
    public:
        DirMaker();
    };
protected:
    DirMaker dirmaker;
    ImmsDb immsdb;
};

#endif
