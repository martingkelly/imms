#ifndef __PLUGIN_H
#define __PLUGIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void imms_setup(char *ch_email, int use_xidle, int use_sloppy);
void imms_spectrum(uint16_t spectrum[256]);
void imms_init();
void imms_cleanup();
void imms_poll();
#ifdef __cplusplus
}
#endif

#endif
