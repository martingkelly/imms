#ifndef __UTILS_H
#define __UTILS_H

#include <sys/time.h>

int imms_random(int max);
time_t usec_diff(struct timeval &tv1, struct timeval &tv2);

#endif
