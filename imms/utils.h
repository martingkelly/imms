#ifndef __UTILS_H
#define __UTILS_H

#include <sys/time.h>
#include <string>

#define     HOUR                    (60*60)
#define     DAY                     (24*HOUR)
#define     ROUND(x)                (int)((x) + 0.5)

using std::string;

int imms_random(int max);
time_t usec_diff(struct timeval &tv1, struct timeval &tv2);

class StackTimer
{
public:
    StackTimer();
    ~StackTimer();
private:
    struct timeval start;
};

float rms_string_distance(const string &s1, const string &s2, int max = INT_MAX);

#endif
