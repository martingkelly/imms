#include <audacious/plugin.h>
#include <audacious/audctrl.h>
#include <audacious/drct.h>
#include <libaudcore/audstrings.h>

#include "immsconf.h"

bool_t init(void);
void about(void);
void configure(void);
void cleanup(void);

/*
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
*/
AUD_GENERAL_PLUGIN(
        .name = PACKAGE_NAME,
        .init = init,
        .cleanup = cleanup,
        .about = about,
        .configure = configure
)
