#ifndef __PLUGIN_H
#define __PLUGIN_H

#include "immsconf.h"

#ifdef __cplusplus
extern "C" {
#endif

void imms_setup(int use_xidle);
void imms_init();
void imms_cleanup();
void imms_poll();
#ifdef __cplusplus
}
#endif

#endif
