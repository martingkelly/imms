#include "xidle.h"

#include <iostream>

#ifdef WITH_XSCREENSAVER
#include <X11/extensions/scrnsaver.h>
#endif

using std::cerr;
using std::endl;

#define SAMPLE_RATE     10
#define MIN_ACTIVE      1

XIdle::XIdle() : xidle_enabled(true), prev_mask(0),
                     prev_rootX(-1), prev_rootY(-1)
{
    if ((display = XOpenDisplay(0)))
    {
        root = DefaultRootWindow(display);
        screen = ScreenOfDisplay(display, DefaultScreen(display));
    }
    else
    {
        cerr << "IMMS: Could not open X display." << endl;
        cerr << "IMMS: Disabling idleness detector." << endl;
    }

    reset();
}

void XIdle::reset()
{
    last_checked = time(0);
    active = 0;
}

bool XIdle::is_active()
{
    return active > MIN_ACTIVE;
}

void XIdle::query()
{
    if (!xidle_enabled)
        return;

    if (active > MIN_ACTIVE || !display)
        return;

    if (time(0) < last_checked + SAMPLE_RATE)
        return;

    if (!query_idle_time())
        query_pointer();

    last_checked = time(0);
}

bool XIdle::query_idle_time()
{
    time_t idle_time = 1000;

#ifdef WITH_XSCREENSAVER
    static XScreenSaverInfo* mitInfo = 0;
    if (!mitInfo)
        mitInfo = XScreenSaverAllocInfo();
    XScreenSaverQueryInfo(display, DefaultRootWindow(display), mitInfo);
    idle_time = mitInfo->idle;
#endif 

    if (idle_time < SAMPLE_RATE)
        return ++active;

    return false;
}

bool XIdle::query_pointer()
{
    Window dummyWin;
    unsigned int mask;
    int rootX, rootY, dummyInt;

    if (!XQueryPointer(display, root, &root, &dummyWin, &rootX, &rootY,
                &dummyInt, &dummyInt, &mask))
    {
        // Pointer has moved to another screen,
        // so let's find out which one.
        for (int i = -1; ++i < ScreenCount(display); )
        {
            if (root == RootWindow(display, i))
            {
                screen = ScreenOfDisplay(display, i);
                break;
            }
        }
    }

    if (rootX != prev_rootX || rootY != prev_rootY || mask != prev_mask)
    {
        prev_rootX = rootX;
        prev_rootY = rootY;
        prev_mask = mask;

        return ++active;
    }

    return false;
}
