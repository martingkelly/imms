#ifndef __PICKER_H
#define __PICKER_H

#include <string>
#include <list>
#include <vector>

#include "immsconf.h"
#include "fetcher.h"

#define     MAX_RATING              150
#define     MIN_RATING              75

class SongPicker : protected InfoFetcher
{
public:
    SongPicker();
    virtual int select_next();
    void playlist_ready() { playlist_known = 1; }
    virtual void playlist_changed(int length);

protected:
    bool add_candidate(bool urgent = false);
    void revalidate_current(int pos, const std::string &path);
    void do_events();
    void reset();

    virtual void request_playlist_item(int index) = 0;
    virtual void get_metacandidates() = 0;

    SongData current;
    std::vector<int> metacandidates;

private:
    int next_candidate();
    void get_related(int pivot_sid, int limit);

    int acquired, attempts, playlist_known;
    SongData winner;

    typedef std::list<SongData> Candidates;
    Candidates candidates;
};

#endif
