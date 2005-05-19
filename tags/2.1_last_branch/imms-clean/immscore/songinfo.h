#ifndef __SONGINFO_H
#define __SONGINFO_H

#include "immsconf.h"

#include <string>

using std::string;

class InfoSlave
{
public:
    virtual string get_artist() { return ""; }
    virtual string get_title()  { return ""; }
    virtual string get_album()  { return ""; }
    virtual ~InfoSlave() {};
};

class SongInfo : public InfoSlave
{
public:
    SongInfo() : filename(""), myslave(0) { };
    SongInfo(const string &_filename) { link(_filename); }
    ~SongInfo() { delete myslave; }

    virtual string get_artist();
    virtual string get_title();
    virtual string get_album();

    void link(const string &_filename);

protected:
    string filename;
    InfoSlave *myslave;
};

#endif
