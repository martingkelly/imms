#include <stdlib.h>

#include <iostream>
#include <algorithm>

#include "fetcher.h"
#include "player.h"
#include "strmanip.h"
#include "md5digest.h"
#include "utils.h"

using std::endl;
using std::cerr;


InfoFetcher::SongData::SongData(int _position, const string &_path)
    : position(_position), path(path_simplifyer(_path))
{
    rating = relation = 0;
    identified = unrated = false;
    last_played = 0;
}

bool InfoFetcher::playlist_identify_item(int pos)
{
    string path = ImmsDb::get_playlist_item(pos);

    ImmsDb::identify(path);
    ImmsDb::playlist_update_identity(pos);

    return true;
}

void InfoFetcher::playlist_changed()
{
    StackTimer t;
    ImmsDb::playlist_clear();

    for (int i = 0; i < Player::get_playlist_length(); ++i)
    {
        string path = path_simplifyer(Player::get_playlist_item(i));
        ImmsDb::playlist_insert_item(i, path);
    }
}

bool InfoFetcher::fetch_song_info(SongData &data)
{
    if (!ImmsDb::playlist_id_from_item(data.position))
        if (!playlist_identify_item(data.position))
            return false;

    const string &path = data.path;

    if (!access(path.c_str(), R_OK))
        return false;

    StringPair info = ImmsDb::get_info();

    string artist = info.first;
    string title = info.second;

    if (artist != "" && title != "")
    {
        data.identified = true;
    }
    else
    {
        if ((data.identified = parse_song_info(path, title)))
            ImmsDb::set_title(title);
    }

    data.rating = ImmsDb::get_rating();

    data.unrated = false;
    if (data.rating < 1)
    {
        data.unrated = true;
        data.rating = ImmsDb::avg_rating(artist, title);
        if (data.rating < 1)
            data.rating = 100;

        ImmsDb::set_rating(data.rating);
    }

#ifdef DEBUG
#if 0
    cerr << "path:\t" << path << endl;
    cerr << "artist:\t" << artist << (identified ? " *" : "") << endl;
    cerr << "title:\t" << title << (identified ? " *" : "") << endl;
#endif
#endif

    *((Song*)&data) = *((Song*)this);

    data.last_played = (data.get_sid() != next_sid) ?
                            time(0) - ImmsDb::get_last() : 0;
    return true;
}

bool InfoFetcher::parse_song_info(const string &path, string &title)
{

#define REMIXCLUES "rmx|mix|[^a-z]version|edit|original|remaster|" \
    "cut|instrumental|extended"
#define BADARTIST "artist|^va$|various|collection|^misc"

    //////////////////////////////////////////////
    // Initialize

    bool identified = false;
    bool parser_confident, artist_confirmed = false;

    link(path);

    string artist = string_normalize(get_artist());
    string tag_artist = artist;

    StringPair fm = get_simplified_filename_mask(path);
    fm.second = string_normalize(fm.second);

    list<string> file_parts;
    parser_confident = imms_magic_parse_filename(file_parts, fm.first);

    // Can't normalize before we pass it to magic parse
    fm.first = string_normalize(fm.first);

    list<string> path_parts;
    imms_magic_parse_path(path_parts, path_get_dirname(path));

#ifdef DEBUG
#if 0
    cerr << "path parts: " << endl;
    for (list<string>::iterator i = path_parts.begin();
            i != path_parts.end(); ++i)
    {
        cerr << " >> " << *i << endl;
    }
    cerr << "name parts: " << endl;
    for (list<string>::iterator i = file_parts.begin();
            i != file_parts.end(); ++i)
    {
        cerr << " >> " << *i << endl;
    }
#endif
#endif

    //////////////////////////////////////////////
    // Try to identify the artist
    
    bool bad_tag_artist = tag_artist == "" || Regexx(tag_artist, BADARTIST);

    // See if we can recognize any of the path parts
    if (file_parts.size() > 1)
    {
        for (list<string>::iterator i = file_parts.begin();
                !artist_confirmed && i != file_parts.end(); ++i)
        {
            if (ImmsDb::check_artist(*i)
                    || (!bad_tag_artist && string_like(*i, tag_artist, 4)))
            {
                artist_confirmed = true;
                artist = *i;
            }
        }
    } 

    // See if any of the directories look familiar
    string outermost = "";
    for (list<string>::iterator i = path_parts.begin();
            !artist_confirmed && i != path_parts.end(); ++i)
    {
        if (Regexx(*i, BADARTIST))
            continue;
        if (artist_confirmed =
                (ImmsDb::check_artist(*i) || string_like(*i, artist, 4)))
            artist = *i;
        // And while we are at it find the uppermost related directory
        else if (outermost == "" && fm.first.find(*i) != string::npos)
            outermost = *i;
    }

    // The path part that is also the outermost directory might be the artist
    if (!artist_confirmed && outermost != "" && fm.second.find(outermost) < 2)
    {
        artist = outermost;
        artist_confirmed = true;
    }

    if (!artist_confirmed && parser_confident && file_parts.size() > 1)
    {
        artist = file_parts.front();
        artist_confirmed = true;
    }

    // Give the tag artist an opportunity to override the guessed artist
    if (!bad_tag_artist && !string_like(tag_artist, artist, 4))
    {
        if (ImmsDb::check_artist(tag_artist)
                || fm.first.find(tag_artist) != string::npos)
        {
            artist = tag_artist;
            artist_confirmed = true;
        }
    }

    // If we still don't know what to do - give up
    if (!artist_confirmed)
        return identified;

    // Erase the first occurrence of the artist
    list<string>::iterator i = find(file_parts.begin(), file_parts.end(),
            artist);
    if (i != file_parts.end())
        file_parts.erase(i);

    // By this point the artist had to have been confirmed
    ImmsDb::set_artist(artist); 
    //////////////////////////////////////////////
    // Try (not very hard) to identify the album

    string album = album_filter(get_album());
    string directory = album_filter(path_parts.back());
    if (album == "" || Regexx(directory, album))
        album = directory;

    //////////////////////////////////////////////
    // Try to identify the title

    string tag_title = string_tolower(get_title());
    title = title_filter(tag_title);
    // If we know the title and were only missing the artist
    if (title != "" && (identified = ImmsDb::check_title(artist, title)))
        return identified;

    if (identified = Regexx(title, album + ".*(" REMIXCLUES ")"))
        title = album;
    else if (identified = Regexx(fm.first, album + "$"))
        title = album;

    // Can recognize the title by matching filename parts to the database?
    for (list<string>::reverse_iterator i = file_parts.rbegin();
            !identified && i != file_parts.rend(); ++i)
    {
        if (ImmsDb::check_title(artist, *i) || string_like(*i, title, 4))
        {
            identified = true;
            title = *i;
        }
        else if (identified = Regexx(*i, album + ".*(" REMIXCLUES ")"))
            title = album;
    }

    if (identified)
        return identified;

    // See if we can trust the tag title enough to just default to it
    if (title != "" && !Regexx(title, "(" REMIXCLUES "|^track$|^title$)")
            && tag_artist != "" && !Regexx(tag_artist, BADARTIST)
            && string_like(tag_title, title, 6))
    {
        return identified = true;
    }

    // Filter things that are probably not the title
    for (list<string>::iterator i = file_parts.begin();
            i != file_parts.end(); ++i)
    {
        if (Regexx(*i, "(" REMIXCLUES "|^track$|^title$)"))
            *i = "###";
    }

    file_parts.remove("###");

    // Default to the right most non-suspicious entry as the title
    if (parser_confident && file_parts.size())
    {
        identified = true;
        title = file_parts.back();
    }
    // nothing left. maybe a remix album??
    else if (!file_parts.size() && ImmsDb::check_title(artist, album))
    {
        identified = true;
        title = album;
    }
    return identified;
}
