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
    int  select_next();
    bool add_candidate(int position, std::string path,
            bool urgent = false);

protected:
    void revalidate_winner(const std::string &path);
    void reset();

    enum { ON, AUTOOFF, OFF } state;
    SongData winner;

private:
    int acquired, attempts;

    typedef std::list<SongData> Candidates;
    Candidates candidates;
};

#endif
