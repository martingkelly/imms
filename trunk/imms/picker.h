#ifndef __PICKER_H
#define __PICKER_H

#include <string>
#include <list>

#include "fetcher.h"

#define     MAX_RATING              150
#define     MIN_RATING              75

using std::string;
using std::list;

class SongPicker : protected InfoFetcher
{
public:
    int  select_next();
    bool add_candidate(int playlist_num, string path, bool urgent = false);
protected:
    void revalidate_winner(const string &path);
    int have_candidates, attempts;

    typedef list<SongData> Candidates;
    Candidates candidates;
    SongData winner;
};

#endif
