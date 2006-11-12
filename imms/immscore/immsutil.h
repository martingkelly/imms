#ifndef __UTILS_H
#define __UTILS_H

#include <sys/time.h>
#include <string>
#include <vector>
#include <stdint.h>

#include "appname.h"

enum LogTypes {
    INFO,
    ERROR
};

#define     HOUR                    (60*60)
#define     DAY                     (24*HOUR)
#define     ROUND(x)                (int)((x) + 0.5)
#define     DIVROUNDUP(x,y)         ((int)(x + y - 1) / (int)y)

#define     LOG(x)                  \
            (x == INFO ? std::cout : std::cerr) << AppName << ": "
#define     DEBUGVAL(x)             LOG(ERROR) << #x << " = " << x << endl;

using std::string;
using std::vector;

int imms_random(int max);
uint64_t usec_diff(struct timeval &tv1, struct timeval &tv2);

static inline float cap(float val, float max = 1) {
    return std::max(std::min(val, max), -max);
}

int socket_connect(const string &sockname);

class StackTimer
{
public:
    StackTimer();
    ~StackTimer();
private:
    struct timeval start;
};

class StackLockFile
{
public:
    StackLockFile(const string &_name);
    ~StackLockFile();
    bool isok() { return name != ""; }
private:
    string name;
};

string get_imms_root(const string &file = "");

string path_normalize(const string &path);

float rms_string_distance(const string &s1, const string &s2,
        int max = INT_MAX);
int listdir(const string &dirname, vector<string> &files);
bool file_exists(const string &filename);

#endif
