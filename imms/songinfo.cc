#include "songinfo.h"
#include "strmanip.h"

#include <stdio.h>
#include <string.h>

#ifdef WITH_VORBIS
# include <vorbis/codec.h>
#endif

// SongInfo

SongInfo::SongInfo() : filename(""), myslave(0) { }

void SongInfo::link(string _filename)
{
    if (filename == _filename)
        return;

    filename = _filename;

    delete myslave;
    myslave = 0;

    if (filename.length() > 3)
    {
        string ext = string_tolower(path_get_extension(filename));

        if (false) {}
#ifdef WITH_ID3LIB
        else if (ext == "mp3")
            myslave = new Mp3Info(filename);
#endif
#ifdef WITH_VORBIS
        else if (ext == "ogg")
            myslave = new OggInfo(filename);
#endif
    }

    if (!myslave)
        myslave = new InfoSlave();
}

#ifdef WITH_ID3LIB
// Mp3Info

Mp3Info::Mp3Info(const string &filename)
{
    id3tag.Clear();
    id3tag.Link(filename.c_str());
}

string Mp3Info::get_text_frame(ID3_FrameID id)
{
    static char buffer[1024];
    ID3_Frame *myFrame = id3tag.Find(id);
    if (myFrame)
    {
        myFrame->Field(ID3FN_TEXT).Get(buffer, 1024);
        return buffer;
    }
    return "";
}

#ifdef LEGACY_RATINGS
int Mp3Info::get_rating(const string &email)
{
    ID3_Frame* popmframe = id3tag.Find(ID3FID_POPULARIMETER,
            ID3FN_EMAIL, email.c_str());

    if (popmframe)
    {
        ID3_Field* ratingfield = popmframe->GetField(ID3FN_RATING);
        if (ratingfield && ratingfield->Get() > 0)
        {
            return ratingfield->Get();
        }
    }
    return INITIAL_RATING;
}
#endif

#endif

#ifdef WITH_VORBIS
// OggInfo

OggInfo::OggInfo(const string &filename)
{
    comment = 0;

    FILE *fh = fopen(filename.c_str(), "r");
    if (!fh)
        return;

    if (ov_open(fh, &vf, NULL, 0))
        return;

    comment = ov_comment(&vf, -1);
}

OggInfo::~OggInfo()
{
    vorbis_comment_clear(comment);
    ov_clear(&vf);
}

string OggInfo::get_comment(const string &id)
{
    // Why exactly does vorbis_comment_query take a non-const char* ??
    char *content = 0, *tag = const_cast<char*>(id.c_str());
    return (comment &&
            (content = vorbis_comment_query(comment, tag, 0))) ?  content : "";
}
#endif
