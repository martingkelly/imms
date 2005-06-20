#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include <map>
#include <set>
#include <string>
#include <iostream>

#include <musicbrainz/mb_c.h>

#include <immsutil.h>
#include <appname.h>
#include <songinfo.h>
#include <immsdb.h> 
#include <analyzer/soxprovider.h> 

#define SAMPLERATE      22050
#define NUMSAMPLES      256
#define DURATIONACC     5

using std::cout;
using std::cerr;
using std::endl;
using std::set;
using std::string;
using std::multimap;

typedef uint16_t sample_t;

const string AppName = "autotag";

static const string separators = "-_[]() ";

class AutoTag
{
    struct MBSongInfo
    {
        MBSongInfo() { duration = track = -1; score = 0; }
        string artist, album, title;
        time_t duration;
        int track;
        int score;

        bool operator==(const MBSongInfo &other)
        {
            return artist == other.artist
                && album == other.album
                && title == other.title
                && track == other.track;
        }

        void extract_words(set<string> &result)
        {
            list<string> allwords;
            string_split(allwords, title, separators);
            string_split(allwords, album, separators);
            string_split(allwords, artist, separators);

            if (track > 0)
                allwords.push_back(itos(track));

            for (list<string>::iterator i = allwords.begin();
                    i != allwords.end(); ++i)
                result.insert(string_tolower(*i));
        }

        void print()
        {
             DEBUGVAL(artist);
             DEBUGVAL(album);
             DEBUGVAL(title);
             DEBUGVAL(track);
             DEBUGVAL(duration);
        }
    };
public:
    int autotag(const string &path)
    {
        string trm;
        time_t length = get_trm(path, trm);
        if (length < 0)
            return length;

        DEBUGVAL(path);
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

        multimap <int, MBSongInfo*> sortedinfo;
        for (size_t i = 0; i < infos.size(); ++i)
            sortedinfo.insert(
                    pair<int, MBSongInfo*>(infos[i].score, &infos[i]));

        for (multimap <int, MBSongInfo*>::iterator i = sortedinfo.begin();
                i != sortedinfo.end(); ++i)
        {
            LOG(ERROR) << string(80, '-') << endl;
            DEBUGVAL(i->first);
            i->second->print();
        }

        return 0;
    }
protected:
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

        string_split(allwords, tag.get_title(), separators);
        string_split(allwords, tag.get_album(), separators);
        string_split(allwords, tag.get_artist(), separators);

        for (list<string>::iterator i = allwords.begin();
                i != allwords.end(); ++i)
        {
            string s = string_tolower(*i);
            if (s.length() == 2 && s[0] == '0' && isdigit(s[1]))
                s = s[1];
            if (s.length() == 3 && isdigit(s[0])
                    && isdigit(s[1]) && isdigit(s[2]))
            {
                int i = s[0] - '0';
                if (i >= 0 && i < 3)
                {
                    string other = s.substr(1);
                    result.insert(other);
                }
            }
            result.insert(s);
        }

        for (set<string>::iterator i = result.begin();
                i != result.end(); ++i)
            DEBUGVAL(*i);
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

             if (mb_GetResultData(m, MBE_AlbumGetAlbumName,
                         strbuf, sizeof(strbuf)))
                 info.album = strbuf;

             bool merged = false;
             for (unsigned i = 0; i < infos.size(); ++i)
                 if (infos[i] == info && abs(infos[i].duration
                             - info.duration) < DURATIONACC)
                 {
                    infos[i].duration = (infos[i].duration + info.duration) / 2;
                    merged = true;
                    break;
                 }

             if (!merged)
                 infos.push_back(info);
         }

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
private:
    SongInfo tag;
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
