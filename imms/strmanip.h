#ifndef __STRMANIP_H
#define __STRMANIP_H

#include <string>
#include <sstream>
#include <list>
#include <utility>
#include <ctype.h>

#include "regexx.h"
#include "levenshtein.h"

using std::string;
using std::ostringstream;
using std::pair;
using std::list;

using namespace regexx;

extern Regexx rex;

template <class T>
inline string itos(T i)
{
    ostringstream str;
    str << i;
    return str.str();
};

// Double up single quotes to escape them sqlite style
inline string escape_string(const string &in)
{
    return rex.replace(in, "'", "''", Regexx::global); 
}

inline string path_simplifyer(string path)
{
    path = rex.replace(path, "/\\.?/", "/", Regexx::global); 
    path = rex.replace(path, "/[^/]+/\\.\\./", "/", Regexx::global); 
    return path;
}


inline string strtime(double mins)
{
    int hours = (int)mins / 60;
    if (!hours)
        return "0h";

    ostringstream s;
    if (hours > 23)
    {
        if (hours / 24)
            s << hours / 24 << "d";
        hours %= 24;
    }
    if (hours)
        s << hours << "h";
    return s.str();
};               

inline string string_tolower(string s)
{
    for (string::iterator i = s.begin(); i != s.end(); i++)
       *i = tolower(*i);

    return s;
};

inline string string_brfilter(string s)
{
    int brackets = 0;
    string news = "";
    for (string::iterator i = s.begin(); i != s.end(); i++)
    {
        char c = *i;
        if (c == '[' || c == '{' || c == '(')
        {
            brackets++;
            // Replace brackets with a "/" to act as a separator
            news += "/";
        }
        if (!brackets)
        {
            news += *i;
        }
        if ((c == ']' || c == '}' || c == ')') && brackets > 0)
        {
            brackets--;
        }
    }
    return news;
};            

string string_normalize(string s);
string title_filter(const string &title);
string album_filter(const string &album);
string string_delete(const string &haystack, const string &needle);

void imms_magic_parse_path(list<string> &store, string path);
bool imms_magic_parse_filename(list<string> &store, string filename);

string path_get_filename(const string &path);
string path_get_dirname(const string &path);
string path_get_extension(const string &path);

bool string_like(const string &s1, const string &s2, int slack = 0);
pair<string, string> get_simplified_filename_mask(const string &path);

#endif
