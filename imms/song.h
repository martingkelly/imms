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
    Song(const string &path);

    void set_last(time_t last);
    void set_title(const string &_title);
    void set_artist(const string &_artist);
    void set_rating(int rating);
    void set_acoustic(const string &spectrum, const string &bpmgraph);
    void increment_playcounter();

    int get_rating();
    time_t get_last();
    int get_playcounter();

    StringPair get_info();
    StringPair get_acoustic();

    int get_sid() { return sid; }
    int get_uid() { return uid; }
    const string &get_path() { return path; }

    bool isok() { return uid != -1 && path != ""; }

    void reset() { playcounter = uid = sid = -1; artist = title = ""; }
protected:
    void register_new_sid();

    int uid, sid, playcounter;
    string title, artist, path;
};

#endif
