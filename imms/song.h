#ifndef __IDENTIFY_H
#define __IDENTIFY_H

#include <utility>
#include <string>

using std::pair;
using std::string;

class Song
{
protected:
    Song() { reset(); }
public:
    static Song identify(const string &path);
    int uid, sid;
    void reset() { uid = sid = 0; }
};

#endif
