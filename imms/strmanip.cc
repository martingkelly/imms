#include "strmanip.h"

#include <sys/types.h>
#include <dirent.h>
#include <string.h>

using std::list;

Regexx rex;

string extradelims;

#define DELIM "[-\\s_\\.'\\(\\)\\[\\]]"
#define NUMRE "(^|" DELIM "+)(\\d+)($|" DELIM "+)"

void string_split(list<string> &store, const string &s, const string &delims)
{
    string expr("(?>[^" + delims + "]+)");
    rex.exec(s, expr, Regexx::global);
    store.insert(store.begin(), rex.match.begin(), rex.match.end());
}

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

bool imms_magic_parse_filename(list<string> &store, string filename)
{
    bool confident = true;

    filename = rex.replace(filename, "[-\\s_\\.]{2,}", "/");

    if (!rex.matches() && (extradelims != "" || !rex.exec(filename, " ")))
    {
        if (extradelims != "")
            filename = rex.replace(filename, "[" + extradelims + "]",
                    "/", Regexx::global);
        confident = (extradelims != "" || !rex.matches());
    }

    imms_magic_parse_path(store, filename);
    return confident;
}

void imms_magic_parse_path(list<string> &store, string path)
{
    path = string_tolower(path);
    path = rex.replace(path, "[-\\s_\\.]{2,}", "/", Regexx::global);
    path = rex.replace(path, "(/|^)[\\(\\[]", "/", Regexx::global);
    path = rex.replace(path, "[\\(\\[][^/]+[\\)\\]]/", "/", Regexx::global);
    path = rex.replace(path, "[-\\s_\\./][iv]{2}i?[/$]", "/", Regexx::global);
    path = rex.replace(path, "[^a-z/]", "", Regexx::global);
    string_split(store, path, "/");
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

    list<string> files;

    DIR *dir = opendir(dirname.c_str());
    struct dirent *de;
    while ((de = readdir(dir)))
    {
        if (de->d_name[0] != '.' && path_get_extension(de->d_name) == extension)
            files.push_back(filename_cleanup(path_get_filename(de->d_name)));
    }
    closedir(dir);

    char *mask = new char[filename.length() + 1];
    memset(mask, 0, filename.length() + 1);

    int count = 0;
    for (list<string>::iterator i = files.begin(); i != files.end(); i++)
    {
        count++;
        size_t num_blocks;
        LevMatchingBlock *blocks = get_matching_blocks(filename, *i, num_blocks);

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

    if (last_dot == string::npos)
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
