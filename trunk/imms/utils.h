#ifndef __UTILS_H
#define __UTILS_H

#include <sys/time.h>

#define     HOUR                    (60*60)
#define     DAY                     (24*HOUR)
#define     ROUND(x)                (int)((x) + 0.5)

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

#endif
