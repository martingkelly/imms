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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "immsconf.h"
#include "immsutil.h"

using std::ifstream;
using std::ofstream;
using std::endl;
using std::ios_base;

// Random
int imms_random(int max)
{
    int rand_num;
    static bool initialized = false;
#ifndef INITSTATE_BUG
    static struct random_data rand_data;
    static char rand_state[256];
    if (!initialized)
    {
        initstate_r(time(0), rand_state, sizeof(rand_state), &rand_data);
        initialized = true;
    }
    random_r(&rand_data, &rand_num);
#else
    if (!initialized)
    {
        srandom(time(0));
        initialized = true;
    }
    rand_num = random();
#endif
    double cof = rand_num / (RAND_MAX + 1.0);
    return (int)(max * cof);
}

uint64_t usec_diff(struct timeval &tv1, struct timeval &tv2)
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
        char *immsroot = getenv("IMMSROOT");
        if (immsroot)
        {
            dotimms = immsroot;
            dotimms.append("/");
        }
        else
        {
            dotimms = getenv("HOME");
            dotimms.append("/.imms/");
        }
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

int socket_connect(const string &sockname)
{
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un sun;
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, sockname.c_str(), sizeof(sun.sun_path));
    if (connect(fd, (sockaddr*)&sun, sizeof(sun)))
    {
        close(fd);
        return -1;
    }
    return fd;
}
