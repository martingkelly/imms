#include "songinfo.h"
#include "strmanip.h"

#include <stdio.h>

#ifdef WITH_ID3LIB
# include <id3/tag.h>

class Mp3Info : public InfoSlave
{
public:
    Mp3Info(const string &filename);
    {
        id3tag.Clear();
        id3tag.Link(filename.c_str());
    }
    virtual string get_artist()
        { return get_text_frame(ID3FID_LEADARTIST); }
    virtual string get_title()
        { return get_text_frame(ID3FID_TITLE); }
    virtual string get_album()
        { return get_text_frame(ID3FID_ALBUM); }
protected:
    string get_text_frame(ID3_FrameID id);
    ID3_Tag id3tag;
};

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
#endif

#ifdef WITH_VORBIS
# include <vorbis/vorbisfile.h>
# include <vorbis/codec.h>

class OggInfo : public InfoSlave
{
public:
    OggInfo(const string &filename);

    virtual string get_artist()
        { return get_comment("artist"); }
    virtual string get_title()
        { return get_comment("title"); }
    virtual string get_album()
        { return get_comment("album"); }

    ~OggInfo();
private:
    string get_comment(const string &id);

    OggVorbis_File vf;
    vorbis_comment *comment;
};

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

#ifdef WITH_TAGLIB

#include <fileref.h>
#include <tag.h>

using namespace TagLib;

class TagInfo : public InfoSlave
{
    public:
        TagInfo(const string &filename)
            : fileref(filename.c_str(), false) {}

        virtual string get_artist()
        {
            return !fileref.isNull() && fileref.tag() ? 
                    fileref.tag()->artist().toCString() : "";
        }
        virtual string get_title()
        {
            return !fileref.isNull() && fileref.tag() ? 
                    fileref.tag()->title().toCString() : "";
        }
        virtual string get_album()
        {
            return !fileref.isNull() && fileref.tag() ? 
                    fileref.tag()->album().toCString() : "";
        }
    private:
        FileRef fileref;
};
#endif

// SongInfo
void SongInfo::link(const string &_filename)
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
#ifdef WITH_TAGLIB
        else if (1)
            myslave = new TagInfo(filename);
#endif
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
