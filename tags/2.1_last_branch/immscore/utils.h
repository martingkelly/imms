#ifndef __UTILS_H
#define __UTILS_H

#include <sys/time.h>
#include <string>
#include <vector>
#include <stdint.h>

#define     HOUR                    (60*60)
#define     DAY                     (24*HOUR)
#define     ROUND(x)                (int)((x) + 0.5)

using std::string;
using std::vector;

int imms_random(int max);
uint64_t usec_diff(struct timeval &tv1, struct timeval &tv2);

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

float rms_string_distance(const string &s1, const string &s2, int max = INT_MAX);
string rescale_bpmgraph(const string &graph);
int listdir(const string &dirname, vector<string> &files);

#endif
