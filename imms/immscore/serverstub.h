#ifndef __SERVERSTUB_H
#define __SERVERSTUB_H

#include <string>

using std::string;

class IMMSServer
{
public:
    void request_playlist_change();
    void request_playlist_item(int index);
    void request_entire_playlist();
    void reset_selection();

    virtual void playlist_updated() = 0;
protected:
    virtual void write_command(const string &line) = 0;

};

#endif
