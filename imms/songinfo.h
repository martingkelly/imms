#ifndef __SONGINFO_H
#define __SONGINFO_H

#include "immsconf.h"

#include <string>

#ifdef WITH_ID3LIB
# include <id3/tag.h>
#endif
#ifdef WITH_VORBIS
# include <vorbis/vorbisfile.h>
#endif

#define INITIAL_RATING 100

using std::string;

class InfoSlave
{
public:
    virtual string get_artist()
        { return ""; }
    virtual string get_title()
        { return ""; }
    virtual string get_album()
        { return ""; }
    virtual int get_rating(const string &email)
        { return INITIAL_RATING; }
    virtual ~InfoSlave() {};
};

class SongInfo : public InfoSlave
{
public:
    SongInfo();

    virtual string get_artist()
        { return myslave->get_artist(); }
    virtual string get_title()
        { return myslave->get_title(); }
    virtual string get_album()
        { return myslave->get_album(); }
    virtual int get_rating(const string &email)
        { return myslave->get_rating(email); }

    void link(string _filename);

    const string &get_filename()
        { return filename; }
protected:
    string filename;
    InfoSlave *myslave;
};

#ifdef WITH_ID3LIB
class Mp3Info : public InfoSlave
{
public:
    Mp3Info(const string &filename);
    virtual string get_artist()
        { return get_text_frame(ID3FID_LEADARTIST); }
    virtual string get_title()
        { return get_text_frame(ID3FID_TITLE); }
    virtual string get_album()
        { return get_text_frame(ID3FID_ALBUM); }
    virtual int get_rating(const string &email);
protected:
    string get_text_frame(ID3_FrameID id);
    ID3_Tag id3tag;
};
#endif

#ifdef WITH_VORBIS
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
#endif

#endif
