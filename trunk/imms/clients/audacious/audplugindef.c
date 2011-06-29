#include <audacious/plugin.h>
#include <audacious/audctrl.h>
#include <audacious/drct.h>
#include <libaudcore/audstrings.h>

#include "immsconf.h"

void init(void);
void about(void);
void configure(void);
void cleanup(void);

static GeneralPlugin imms_gp =
{
    PACKAGE_STRING,
    init,
    cleanup,
    about,
    configure,
    NULL,
    NULL,
};

GeneralPlugin *gp_plugin_list[] = { &imms_gp, NULL };

SIMPLE_GENERAL_PLUGIN(imms, gp_plugin_list);
