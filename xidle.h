#ifndef __XIDLE_H
#define __XIDLE_H

#include "immsconf.h"

#include <time.h>
#include <X11/Xlib.h>

#define SAMPLE_RATE     10
#define MIN_ACTIVE      1

class XIdle
{
public:
    XIdle();
    bool is_active() { return active > MIN_ACTIVE; }
    void reset();
    void query();

private:
    bool query_idle_time();
    bool query_pointer();

    int active;
    time_t last_checked;

    Display *display;
    Screen *screen;
    Window root;

    unsigned int prev_mask;
    int prev_rootX, prev_rootY;
};

#endif
