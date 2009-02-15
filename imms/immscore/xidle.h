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
