#include <stdlib.h>     // for (s)random
#include <signal.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>

#include "immsconf.h"
#include "utils.h"

using std::ifstream;
using std::ofstream;
using std::ios_base;

// Random
int imms_random(int max)
{
    int rand_num;
    static struct random_data rand_data;
    static char rand_state[256];
    static bool initialized = false;
    if (!initialized)
    {
        initstate_r(time(0), rand_state, sizeof(rand_state), &rand_data);
        initialized = true;
    }
    random_r(&rand_data, &rand_num);
    double cof = rand_num / (RAND_MAX + 1.0);
    return (int)(max * cof);
}

time_t usec_diff(struct timeval &tv1, struct timeval &tv2)
{
    return (tv2.tv_sec - tv1.tv_sec) * 1000000
        + tv2.tv_usec - tv1.tv_usec;
}

StackTimer::StackTimer()
{
#ifdef DEBUG
    gettimeofday(&start, 0);
#endif
}

StackTimer::~StackTimer()
{
#ifdef DEBUG
    struct timeval end;
    gettimeofday(&end, 0);
    std::cout << usec_diff(start, end) / 1000 << " msecs elapsed" << std::endl;
#endif
}

string get_imms_root(const string &file)
{
    static string dotimms;
    if (dotimms == "")
    {
        dotimms = getenv("HOME");
        dotimms.append("/.imms/");
    }
    return dotimms + file;
}

StackLockFile::StackLockFile(const string &_name) : name(_name)
{
    while (1)
    {
        ifstream lockfile(name.c_str());
        int pid = 0;
        lockfile >> pid;
        if (!pid)
            break;
        if (kill(pid, 0))
            break;
        name = "";
        return;
    }

    ofstream lockfile(name.c_str(), ios_base::trunc | ios_base::out);
    lockfile << getpid() << std::endl;
    lockfile.close();
}

StackLockFile::~StackLockFile()
{
    if (name != "")
        unlink(name.c_str());
}

float rms_string_distance(const string &s1, const string &s2, int max)
{
    if (s1 == "" || s2 == "")
        return 0;

    int len = s1.length();
    if (len != (int)s2.length())
        return 0;
    len = std::min(len, max);
    float distance = 0;

    for (int i = 0; i < len; ++i)
        distance += pow(s1[i] - s2[i], 2);

    return sqrt(distance / len);
}

#define STEP 3

static int do_rescale_sum(const string &graph)
{
    if (graph.length() < STEP * 3)
        return 0;

    float total = 0;
    float scale = 1.0 / (float)STEP;
    for (int i = 0; i < STEP; ++i)
    {
        total += (scale * (i + 0.5) * (graph[i] - 'a'));
        total += (graph[STEP + i] - 'a');
        total += (scale * (STEP - i - 0.5) * (graph[STEP * 2 + i] - 'a'));
    }
    return ROUND(total / (STEP * 2));
}

string rescale_bpmgraph(const string &graph)
{
    string result;
    int cur;
    for (int i = 0; (cur = do_rescale_sum(graph.substr(i))); i += STEP)
        result += std::min((char)('a' + cur), 'z');
    return result;
}

string path_normalize(const string &path)
{
    const char *start = path.c_str();
    while (isspace(*start))
        start++;
    if (access(start, R_OK))
        return start;
    char resolved[4096];
    realpath(start, resolved);
    return resolved;
}

int listdir(const string &dirname, vector<string> &files)
{
    files.clear();
    DIR *dir = opendir(dirname.c_str());
    if (!dir)
        return errno;
    struct dirent *de;
    while ((de = readdir(dir)))
        files.push_back(de->d_name);
    closedir(dir);
    return 0;
}
