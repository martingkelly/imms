#ifndef __IDENTIFY_H
#define __IDENTIFY_H

#include <utility>
#include <string>

using std::pair;
using std::string;

typedef pair<string, string> StringPair;

class Song
{
public:
    Song() { reset(); }
    static Song identify(const string &path);

    void set_last(time_t last);
    void set_title(const string &_title);
    void set_artist(const string &_artist);
    void set_rating(int rating);

    int get_rating();
    time_t get_last();
    StringPair get_info();
    int get_sid() { return sid; }

    void reset() { uid = sid = 0; artist = title = ""; }
protected:
    void register_new_sid();

    int uid, sid;
    string title, artist;
};

#endif
