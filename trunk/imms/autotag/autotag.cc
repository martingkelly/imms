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
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include <map>
#include <set>
#include <string>
#include <iostream>
#include <iomanip>

#include <musicbrainz/mb_c.h>
#include <fileref.h>
#include <tag.h>

#include <immsutil.h>
#include <appname.h>
#include <songinfo.h>
#include <immsdb.h> 
#include <analyzer/soxprovider.h> 

#define SAMPLERATE              22050
#define NUMSAMPLES              256
#define DURATIONACC             5
#define MIN_LEAD_SCORE_DIFF     10
#define SCORE_THRESHOLD_NO_TAG  25

using std::cout;
using std::cerr;
using std::endl;
using std::set;
using std::setw;
using std::ios_base;
using std::string;
using std::multimap;

typedef uint16_t sample_t;

const string AppName = "autotag";

static const string separators = "-_[]() ";

string string_tag(const string &s, const string &tag) {
    return tag + ":" + s;
}

void string_split_tag(list<string> &store, const string &s,
        const string &tag, const string &delims)
{
    list<string> prestore;
    string_split(prestore, s, delims);
    for (list<string>::iterator i = prestore.begin();
            i != prestore.end(); ++i)  {
        store.push_back(string_tag(*i, tag));
        store.push_back(*i);
    }
}

class AutoTag
{
    struct MBSongInfo
    {
        MBSongInfo() { year = duration = track = -1; score = 0; }
        string artist, album, title;
        time_t duration, year;
        int track;
        int score;

        bool operator==(const MBSongInfo &other) const
        {
            return artist == other.artist
                && album == other.album
                && title == other.title
                && track == other.track;
        }
        bool operator< (const MBSongInfo &other) const
        {
            return score > other.score;
        }

        void extract_words(set<string> &result)
        {
            list<string> allwords;
            string_split_tag(allwords, title, "title", separators);
            string_split_tag(allwords, album, "album", separators);
            string_split_tag(allwords, artist, "artist", separators);

            if (track > 0)
                allwords.push_back(string_tag(itos(track), "track"));
            if (year > 0)
                allwords.push_back(itos(year));

            for (list<string>::iterator i = allwords.begin();
                    i != allwords.end(); ++i)
                result.insert(string_tolower(*i));
        }

        void print() const
        {
            const static int colw = 35;
            LOG(INFO) << setiosflags(ios_base::left) 
                << "Artist: " << setw(colw) << artist
                << "Track : " << track << endl;
            LOG(INFO) << "Album : " << setw(colw) << album
                << "Year  : " << year << endl;
            LOG(INFO) << "Title : " << setw(colw) << title
                << "Length: " << duration << endl;
        }
    };
public:
    int autotag(const string &xpath)
    {
        path = xpath;

        string trm;
        time_t length = get_trm(path, trm);
        if (length < 0)
            return length;

        DEBUGVAL(trm);
        DEBUGVAL(length);

        vector<MBSongInfo> infos;
        get_info(trm, infos);

        set<string> filewords;
        parse_info(path, filewords);

        for (size_t i = 0; i < infos.size(); ++i)
        {
            set<string> infowords;
            infos[i].extract_words(infowords);

            set<string> result;
            std::insert_iterator< set<string> > insertiter(result,
                    result.begin());

            set_intersection(filewords.begin(), filewords.end(),
                    infowords.begin(), infowords.end(),
                    insertiter);

            infos[i].score += result.size() * 5;

            if (abs(infos[i].duration - length) < DURATIONACC)
                infos[i].score += 15;
            else if (!infos[i].duration)
                infos[i].score += 5;
            else if (infos[i].duration > length)
                infos[i].score += 2;
            else if (infos[i].duration < length)
                infos[i].score -= 7;
        }

        sort(infos.begin(), infos.end());

        print_infos(infos);

        if (try_tag(infos))
        {
            LOG(ERROR) << "Pass 2: merging... " << endl;
            for (vector<MBSongInfo>::iterator i = infos.begin();
                    i != infos.end(); ++i) {
                i->album = "";
                i->track = -1;
                i->year = -1;
            }
            dedupe(infos);
            print_infos(infos);
            try_tag(infos);
        }

        return 0;
    }

    void print_infos(const vector<MBSongInfo> &infos)
    {
        for (vector<MBSongInfo>::const_reverse_iterator i = infos.rbegin();
                i != infos.rend(); ++i)
        {
            LOG(INFO) << string(60, '-') << endl;
            LOG(INFO) << "- Score: " << i->score << endl;
            i->print();
        }
    }

    void dedupe(vector<MBSongInfo> &infos)
    {
        vector<MBSongInfo> newinfos;
        for (unsigned i = 0; i < infos.size(); ++i) 
        {
            int found = false;
            for (unsigned j = 0; j < newinfos.size(); ++j)
                if (infos[i] == newinfos[j] &&
                        (!infos[i].duration || !newinfos[j].duration ||
                        abs(infos[i].duration - newinfos[j].duration)
                        < DURATIONACC))
                {
                    if (infos[i].duration && newinfos[j].duration)
                        newinfos[j].duration = (infos[i].duration
                                + newinfos[j].duration) / 2;
                    else
                        newinfos[j].duration = std::max(infos[i].duration,
                                newinfos[j].duration);
                    newinfos[j].score = std::max(infos[i].score,
                            newinfos[j].score);
                    found = true;
                    break;
                }
            if (!found)
                newinfos.push_back(infos[i]);
        }
        infos.clear();
        copy(newinfos.begin(), newinfos.end(), back_inserter(infos));
        sort(infos.begin(), infos.end());
    }

    int try_tag(const vector<MBSongInfo> &infos)
    {
        if (infos.size() == 0) {
            LOG(ERROR) << "Sorry, I don't know what this song is..." << endl;
            return 0;
        }

        const MBSongInfo &leader = infos[0];
        if (infos.size() > 1 &&
                leader.score - infos[1].score < MIN_LEAD_SCORE_DIFF) {
            LOG(ERROR) << "Too ambiguious, not tagging" << endl;
            return 1;
        }

        if (leader.score >= SCORE_THRESHOLD_NO_TAG)
        {
            LOG(ERROR) << " --- Tagging with --- " << endl;
            leader.print();
            write_tag(path, leader);
        }
        return 0;
    }
protected:
    bool has_tag() const
    {
        return !tag.get_artist().empty() && !tag.get_title().empty();
    }

    void parse_info(const string &path, set<string> &result)
    {
        list<string> allwords;
        string dirname = path_get_dirname(path);
        string filename = path_get_filename(path);

        string_split(allwords, filename, separators);

        list<string> dirparts;
        string_split(dirparts, dirname, "/");

        int j = 0;
        for (list<string>::reverse_iterator i = dirparts.rbegin();
                i != dirparts.rend() && j < 3; ++i, ++j)
            string_split(allwords, *i, separators);

        string_split_tag(allwords, tag.get_title(), "title", separators);
        string_split_tag(allwords, tag.get_album(), "album", separators);
        string_split_tag(allwords, tag.get_artist(), "artist", separators);

        for (list<string>::iterator i = allwords.begin();
                i != allwords.end(); ++i)
        {
            string s = string_tolower(*i);
            if (s.length() == 2 && s[0] == '0' && isdigit(s[1]))
                s = string_tag(s.substr(1), "track");
            if (s.length() == 3 && isdigit(s[0])
                    && isdigit(s[1]) && isdigit(s[2]))
            {
                int i = s[0] - '0';
                if (i >= 0 && i < 3)
                {
                    string other = s.substr(s[1] == '0' ? 2 : 1);
                    result.insert(string_tag(s.substr(1), "track"));
                }
            }
            result.insert(s);
        }
    }
    int get_info(const string &trm, vector<MBSongInfo> &infos)
    {
         musicbrainz_t m = mb_New();
         mb_UseUTF8(m, 0);
         if (getenv("MB_SERVER"))
             mb_SetServer(m, getenv("MB_SERVER"), 80);

         const char *args[] = { trm.c_str(), 0 };

         int ret = mb_QueryWithArgs(m, MBQ_TrackInfoFromTRMId, (char**)args);
         if (!ret)
         {
             char error[256];
             mb_GetQueryError(m, error, sizeof(error));
             LOG(ERROR) << "MusicBrainz lookup failed: " << error << endl;
             mb_Delete(m);
             return -1;
         }

         for(int index = 1;; index++)
         {
             mb_Select(m, MBS_Rewind);

             // Select the first track from the track list
             if (!mb_Select1(m, MBS_SelectTrack, index))
             {
                 if (index == 1)
                     LOG(ERROR) << "Track info not found!" << endl;
                 break;
             }

             MBSongInfo info;

             mb_GetResultData(m, MBE_TrackGetTrackId, strbuf, 256);
             string uri = strbuf;

             if (mb_GetResultData(m, MBE_TrackGetArtistName,
                         strbuf, sizeof(strbuf)))
                 info.artist = strbuf;

             if (mb_GetResultData(m, MBE_TrackGetTrackName,
                         strbuf, sizeof(strbuf)))
                 info.title = strbuf;

             info.duration = mb_GetResultInt(m,
                     MBE_TrackGetTrackDuration) / 1000;

             mb_Select(m, MBS_SelectTrackAlbum);
             int track = mb_GetOrdinalFromList(m, MBE_AlbumGetTrackList,
                     (char*)uri.c_str());
             if (track > 0 && track < 100)
                 info.track = track;

             if (mb_GetResultInt(m, MBE_AlbumGetNumReleaseDates) > 0 &&
                     mb_Select1(m, MBS_SelectReleaseDate, 1) &&
                     mb_GetResultData(m, MBE_ReleaseGetDate,
                         strbuf, sizeof(strbuf)))
             {
                 info.year = atoi(strbuf);
                 mb_Select(m, MBS_Back);
             }

             if (mb_GetResultData(m, MBE_AlbumGetAlbumName,
                         strbuf, sizeof(strbuf)))
                 info.album = strbuf;

             infos.push_back(info);
         }

         dedupe(infos);

         mb_Delete(m);
         return 0;
    }
    
    time_t get_trm(const string &path, string &trm)
    {
        trm_t t = trm_New();
        if (!trm_SetPCMDataInfo(t, SAMPLERATE, 1, 16))
            return -1;

        FILE *p = run_sox(path, SAMPLERATE, true);

        if (!p)
        {
            LOG(ERROR) << "Could not open pipe!" << endl;
            return -2;
        }

        tag.link(path);
        time_t length = tag.get_length();

        if (length)
            trm_SetSongLength(t, length);

        size_t samples = 0;
        while (fread(pcmdata, sizeof(sample_t), NUMSAMPLES, p) == NUMSAMPLES
                && !trm_GenerateSignature(t, (char*)pcmdata, sizeof(pcmdata)))
            samples += NUMSAMPLES;

        if (trm_FinalizeSignature(t, sig, 0))
        {
            LOG(ERROR) << "FinalizeSignature failed!" << endl;
            return -3;
        }

        trm_ConvertSigToASCII(t, sig, readable);

        trm = string(readable);

        trm_Delete(t);
        
        return samples /= SAMPLERATE;
    }
    static void write_tag(const string &filename, const MBSongInfo &info)
    {
        TagLib::FileRef f(filename.c_str());
        f.tag()->setArtist(info.artist);
        f.tag()->setTitle(info.title);
        if (!info.album.empty())
            f.tag()->setAlbum(info.album);
        if (info.track > 0)
            f.tag()->setTrack(info.track);
        if (info.year > 0)
            f.tag()->setYear(info.year);
        f.save();
    }

private:
    string path;
    mutable SongInfo tag;
    sample_t pcmdata[NUMSAMPLES];
    char strbuf[256];
    char sig[17];
    char readable[37];
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "usage: autotag <filename> [<filename>] ..." << endl;
        return -1;
    }

    // clean up after XMMS
    for (int i = 3; i < 255; ++i)
        close(i);

    AutoTag tagger;
    for (int i = 1; i < argc; ++i)
    {
        if (tagger.autotag(path_normalize(argv[i])))
            LOG(ERROR) << "Could not process " << argv[i] << endl;
    }
}
