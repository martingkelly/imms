#ifndef __PICKER_H
#define __PICKER_H

#include <string>
#include <list>

#include "immsconf.h"
#include "fetcher.h"

#define     MAX_RATING              150
#define     MIN_RATING              75

class SongPicker : protected InfoFetcher
{
public:
    SongPicker();
    int select_next();
    virtual void playlist_changed() = 0;

protected:
    bool add_candidate(bool urgent = false);
    void revalidate_current(const std::string &path);
    void reset();

    SongData current;

private:
    int acquired, attempts;
    SongData winner;

    typedef std::list<SongData> Candidates;
    Candidates candidates;
};

#endif
