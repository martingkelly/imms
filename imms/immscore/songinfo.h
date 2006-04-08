#ifndef __SONGINFO_H
#define __SONGINFO_H

#include <time.h>

#include <string>

#include "immsconf.h"

using std::string;

class InfoSlave
{
public:
    virtual string get_artist() { return ""; }
    virtual string get_title()  { return ""; }
    virtual string get_album()  { return ""; }

    virtual time_t get_length() { return 0; }
    virtual int get_track_num() { return 0; }

    virtual ~InfoSlave() {};
};

class SongInfo : public InfoSlave
{
public:
    SongInfo() : filename(""), myslave(0) { };
    SongInfo(const string &_filename) : myslave(0) { link(_filename); }
    ~SongInfo() { delete myslave; }

    virtual string get_artist();
    virtual string get_title();
    virtual string get_album();
    virtual time_t get_length();
    virtual int get_track_num();

    void link(const string &_filename);

protected:
    string filename;
    InfoSlave *myslave;
};

#endif
