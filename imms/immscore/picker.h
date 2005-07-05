#ifndef __PICKER_H
#define __PICKER_H

#include <string>
#include <list>
#include <vector>

#include "immsconf.h"
#include "fetcher.h"

class SongPicker : protected InfoFetcher
{
public:
    SongPicker();
    virtual int select_next();
    virtual void playlist_ready() { playlist_known = 1; }
    virtual void playlist_changed(int length);

protected:
    bool add_candidate(bool urgent = false);
    void revalidate_current(int pos, const std::string &path);
    bool do_events();
    void reset();

    virtual void request_playlist_item(int index) = 0;
    virtual void get_metacandidates(int size) = 0;

    SongData current;
    std::vector<int> metacandidates;

private:
    void get_related(int pivot_sid, int limit);

    int acquired, attempts, playlist_known;
    SongData winner;

    typedef std::list<SongData> Candidates;
    Candidates candidates;
};

#endif
