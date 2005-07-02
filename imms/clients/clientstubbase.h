#ifndef __CLIENTSTUBBASE_H_
#define __CLIENTSTUBBASE_H_

#include <string>
#include <cerrno>

using std::string;

class IMMSClientStub
{
public:
    void setup(bool use_xidle);
    void start_song(int position, std::string path);
    void end_song(bool at_the_end, bool jumped, bool bad);
    void select_next();
    void playlist_changed(int length);
protected:
    virtual void write_command(const string &line) = 0;
}; 

#endif
