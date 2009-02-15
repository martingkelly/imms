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
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "strmanip.h"
#include "immsutil.h"

using std::list;

Regexx rex;

string extradelims;

#define DELIM "[-\\s_\\.'\\(\\)\\[\\]]"
#define NUMRE "(^|" DELIM "+)(\\d+)($|" DELIM "+)"

string escape_char(char c)
{
    string s(1, c);
    switch (c)
    {
        case '[':
        case ']':
        case '(':
        case ')':
        case '.':
            s = "\\" + s;
        default:
            break;
    }
    return s;
}

void string_split(list<string> &store, const string &s, const string &delims)
{
    string escaped;
    for (unsigned i = 0; i < delims.length(); ++i)
        escaped += escape_char(delims[i]);
    string expr("(?>[^" + escaped + "]+)");
    rex.exec(s, expr, Regexx::global);
    store.insert(store.end(), rex.match.begin(), rex.match.end());
}

string trim(const string &s)
{
    size_t p = s.find_last_not_of(" ");
    return p == string::npos ? s : s.substr(0, p + 1);
}

bool imms_magic_preprocess_filename(string &filename)
{
    filename = rex.replace(filename, "[-\\s_\\.]{2,}", "/");

    bool confident = rex.matches();

    if (confident)
        return true;

    if (extradelims != "")
    {
        filename = rex.replace(filename, "[" + extradelims + "]",
                "/", Regexx::global);
        confident = rex.matches();
    }

    if (!confident)
    {
        int spaces = rex.exec(filename, " ", Regexx::global);
        int dashes = rex.exec(filename, "-", Regexx::global);
        int scores = rex.exec(filename, "_", Regexx::global);

        if ((!spaces || !scores) && dashes && dashes < 3
                && (spaces >= dashes || scores >= dashes))
            filename = rex.replace(filename, "-", "/", Regexx::global);
    }

    return confident;
}

void imms_magic_preprocess_path(string &path)
{
    path = string_tolower(path);
    path = rex.replace(path, "[-\\s_\\.]{2,}", "/", Regexx::global);
    path = rex.replace(path, "(/|^)[\\(\\[]", "/", Regexx::global);
    path = rex.replace(path, "[\\(\\[][^/]+[\\)\\]]/", "/", Regexx::global);
    path = rex.replace(path, "[-\\s_\\./][iv]{2}i?[/$]", "/", Regexx::global);
    path = rex.replace(path, "[^a-z/]", "", Regexx::global);
}

bool imms_magic_parse_filename(list<string> &store, string filename)
{
	bool result = imms_magic_preprocess_filename(filename);
	imms_magic_preprocess_path(filename);
    string_split(store, filename, "/");
    return result;
}

void imms_magic_parse_path(list<string> &store, string path)
{
    path = rex.replace(path, "/+$", "", Regexx::global);

    string lastdir = path_get_filename(path);
    path = path_get_dirname(path);

    imms_magic_preprocess_path(path);
    string_split(store, path, "/");

    imms_magic_preprocess_filename(lastdir);
    imms_magic_preprocess_path(lastdir);
    string_split(store, lastdir, "/");
}

string string_normalize(string s)
{
    s = string_brfilter(string_tolower(s));
    s = rex.replace(s, "[^a-z]", "", Regexx::global);
    return s;
}

bool string_like(const string &s1, const string &s2, int slack)
{
    int len1 = s1.length();
    int len2 = s2.length();

    int distance = lev_edit_distance(len1, s1.c_str(), len2, s2.c_str(), 0);
    return (len1 + len2) / (13 - slack) >= distance;
}

LevMatchingBlock *get_matching_blocks(const string &s1, const string &s2,
        size_t &n)
{
    int len1 = s1.length(), len2 = s2.length();
    size_t num_editops;
    LevEditOp *editops = lev_editops_find(len1, s1.c_str(), len2, s2.c_str(),
            &num_editops);

    LevMatchingBlock *blocks =
        lev_editops_matching_blocks(len1, len2, num_editops, editops, &n);

    free(editops);

    return blocks;
}

string filename_cleanup(const string &s)
{
    return string_tolower(rex.replace(s, "(\\d)", "#", Regexx::global));
}

string get_filename_mask(const string& path)
{
    string dirname = path_get_dirname(path);
    string filename = filename_cleanup(path_get_filename(path));
    string extension = path_get_extension(path);

    vector<string> files;
    if (listdir(dirname, files))
        return "";

    char *mask = new char[filename.length() + 1];
    memset(mask, 0, filename.length() + 1);

    int count = 0;
    for (vector<string>::iterator i = files.begin(); i != files.end(); i++)
    {
        if ((*i)[0] == '.' || path_get_extension(*i) == extension)
            continue;

        count++;
        size_t num_blocks;
        LevMatchingBlock *blocks = get_matching_blocks(filename,
                filename_cleanup(path_get_filename(*i)), num_blocks);

        for (size_t j = 0; j < num_blocks; j++)
        {
            for (size_t k = 0; k < blocks[j].len; k++)
                mask[blocks[j].spos + k]++;
        }

        free(blocks);
        if (count > 20)
            break;
    }

    string strmask = "";
    for (size_t i = 0; i < filename.length(); i++)
    {
        strmask += mask[i] > count * 0.7 ? filename[i] : '*';
    }

    delete[] mask;

    return strmask;
}

struct H
{
    static string filename;
    static string mask;
    static string double_erase(const regexx::RegexxMatch& _match)
    {
        mask.erase(_match.start(), _match.length());
        filename.erase(_match.start(), _match.length());

        return "";
    }
    static string numerals(const regexx::RegexxMatch& _match)
    {
        extradelims = "";
        string replacement = "/";
        int l1 = _match.atom[0].length(), l2 = _match.atom[2].length();

        if (l1 < 2 && l2 < 2)
        {
            if (_match.atom[0].str() != " " && _match.atom[0].str() != "_")
            {
                replacement = _match.atom[0].str();
                if (_match.atom[0].length() == 1)
                    extradelims += escape_char(_match.atom[0].str()[0]);
            }
            if (_match.atom[2].str() != " " && _match.atom[2].str() != "_")
            {
                replacement = _match.atom[2].str();
                if (_match.atom[2].length() == 1)
                    extradelims += escape_char(_match.atom[2].str()[0]);
            }
        }
        else
        {
            replacement = _match.atom[(!!(l1 < l2)) * 2].str();
        }

        mask.replace(_match.start(), _match.length(), replacement);
        filename.replace(_match.start(), _match.length(), replacement);
        return "";
    }
};

string H::filename, H::mask;

pair<string, string> get_simplified_filename_mask(const string &path)
{
    H::filename = string_tolower(path_get_filename(path));
    H::mask = get_filename_mask(path);

    if (rex.exec(H::mask, "(\\)|\\]|\\*[a-z]{0,3})-[a-z0-9]{3,4}$"))
        rex.replacef(H::mask, "-[a-z]{3,4}$", H::double_erase, Regexx::global);

    rex.replacef(H::filename,
            "[-\\s_\\.]*[\\(\\[][^\\]\\)]{0,60}[\\]\\)]?$",
            H::double_erase, Regexx::global);

    do
    {
        rex.replacef(H::filename, NUMRE, H::numerals, Regexx::global);
    } while (rex.matches());

    rex.replacef(H::filename, "^[-\\s_\\.']+|[-\\s_\\.']+$",
            H::double_erase, Regexx::global);

    return pair<string, string>(H::filename, H::mask);
}

string album_filter(const string &album)
{
    return string_normalize(rex.replace(string_tolower(album),
            "(lp|ep|cmd|promo|demo|maxi)$", "", Regexx::global));
}

string title_filter(const string &title)
{
    string normtitle = string_normalize(title);
    size_t p = title.rfind("- ");
    if (p == string::npos)
        return normtitle;
    return string_normalize(title.substr(p));
}

string path_get_dirname(const string &path)
{
    size_t last_slash = path.find_last_of("/") + 1;
    return path.substr(0, last_slash);
}

string path_get_filename(const string &path)
{
    size_t last_slash = path.find_last_of("/") + 1;
    size_t last_dot = path.find_last_of(".");

    if (last_dot == string::npos || last_dot < path.length() - 4)
        last_dot = path.length();

    return path.substr(last_slash, last_dot - last_slash);
}

string path_get_extension(const string &path)
{
    size_t last_dot = path.find_last_of(".");

    if (last_dot == string::npos)
        last_dot = path.length();
    else
        last_dot++;

    return path.substr(last_dot);
}

string string_delete(const string &haystack, const string &needle)
{
    return rex.replace(haystack, needle, "", Regexx::global);
}
