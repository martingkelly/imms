#ifndef __PLUGIN_H
#define __PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif
void imms_setup(char *ch_email, int use_xidle, int use_sloppy);
void imms_init();
void imms_cleanup();
void imms_poll();
#ifdef __cplusplus
}
#endif

#endif
