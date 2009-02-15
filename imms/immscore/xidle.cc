/*
  Borrows from xautolock 2.1
  by Michel Eyckmans (MCE) <eyckmans@imec.be>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include "xidle.h"

#include <iostream>

#ifdef WITH_XSCREENSAVER
# include <X11/extensions/scrnsaver.h>
#endif

using std::cerr;
using std::endl;

#define SAMPLE_RATE     10
#define MIN_ACTIVE      1

XIdle::XIdle() : xidle_enabled(true), prev_mask(0),
                     prev_rootX(-1), prev_rootY(-1)
{
#ifdef WITH_XSCREENSAVER
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
#endif

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
#ifdef WITH_XSCREENSAVER
    if (!xidle_enabled)
        return;

    if (active > MIN_ACTIVE || !display)
        return;

    if (time(0) < last_checked + SAMPLE_RATE)
        return;

    if (!query_idle_time())
        query_pointer();

    last_checked = time(0);
#endif
}

bool XIdle::query_idle_time()
{
#ifdef WITH_XSCREENSAVER
    time_t idle_time = 1000;

    static XScreenSaverInfo* mitInfo = 0;
    if (!mitInfo)
        mitInfo = XScreenSaverAllocInfo();
    XScreenSaverQueryInfo(display, DefaultRootWindow(display), mitInfo);
    idle_time = mitInfo->idle;

    if (idle_time < SAMPLE_RATE)
        return ++active;

#endif 
    return false;
}

bool XIdle::query_pointer()
{
#ifdef WITH_XSCREENSAVER
    Window dummyWin;
    unsigned int mask;
    int rootX = 0, rootY = 0, dummyInt;

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

#endif
    return false;
}
