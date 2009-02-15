/*
 IMMS: Intelligent Multimedia Management System
 Copyright (C) 2001-2009 Michael Grigoriev

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef __IDENTIFY_H
#define __IDENTIFY_H

#include <utility>
#include <string>

using std::pair;
using std::string;

typedef pair<string, string> StringPair;

class MixtureModel;

class Song
{
public:
    struct Rating
    {
        Rating(int mean = -1, int dev = 0) : mean(mean), dev(dev) {}
        string print();
        int sample();
        int mean, dev;
    };

    Song(const string &path = "", int _uid = -1, int _sid = -1);

    void set_last(time_t last);
    void set_info(const StringPair &info);
    void set_rating(const Rating &rating);
    void increment_playcounter();

    int get_rating(Rating *r = 0);
    Rating get_raw_rating();
    time_t get_last();
    int get_playcounter();

    StringPair get_info();
    void get_tag_info(string &artist, string &album, string &title) const;

    int get_sid() { return sid; }
    int get_uid() { return uid; }
    const string &get_path() const { return path; }

    bool isok() { return uid != -1 && path != ""; }
    bool isanalyzed();

    void set_acoustic(const MixtureModel &mm, const float *beats);
    bool get_acoustic(MixtureModel *mm, float *beats) const;

    Rating update_rating();
    void infer_rating();

    void reset() { playcounter = uid = sid = -1; artist = title = ""; }
protected:
    void register_new_sid();
    void identify(time_t modtime);
    void update_tag_info(const string &artist, const string &album,
            const string &title);

    int uid, sid, playcounter;
    string title, artist, path;
private:
    void _identify(time_t modtime, const string &checksum);
};

#endif
