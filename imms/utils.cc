#include <stdlib.h>     // for (s)random
#include <time.h>
#include <iostream>
#include <math.h>

#include "utils.h"

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

StackTimer::StackTimer() { gettimeofday(&start, 0); }

StackTimer::~StackTimer()
{
    struct timeval end;
    gettimeofday(&end, 0);
    std::cout << usec_diff(start, end) / 1000 << " msecs elapsed" << std::endl;
}


float rms_string_distance(const string &s1, const string &s2, int max)
{
    if (s1 == "" || s2 == "")
        return 0;

    int len = s1.length();
    assert(len == (int)s2.length());
    len = std::min(len, max);
    float distance = 0;

    for (int i = 0; i < len; ++i)
        distance += pow(s1[i] - s2[i], 2);

    return sqrt(distance / len);
}
