#ifndef __XIDLE_H
#define __XIDLE_H

#include <time.h>
#include <X11/Xlib.h>

#include "immsconf.h"

class XIdle
{
public:
    XIdle();

protected:
    bool is_active();
    void reset();
    void query();

    bool xidle_enabled;

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
