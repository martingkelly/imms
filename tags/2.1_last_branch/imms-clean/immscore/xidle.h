#ifndef __XIDLE_H
#define __XIDLE_H

#include "immsconf.h"

#include <time.h>
#ifdef WITH_XSCREENSAVER
# include <X11/Xlib.h>
#endif

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

#ifdef WITH_XSCREENSAVER
    Display *display;
    Screen *screen;
    Window root;
#endif

    unsigned int prev_mask;
    int prev_rootX, prev_rootY;
};

#endif
