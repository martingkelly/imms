#ifndef __IMMSBASE_H
#define __IMMSBASE_H

#include "immsdb.h"

#define ROUND(x) (int)((x) + 0.5)

int imms_random(int max);

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
