#include <glib.h>

#include "imms.h"
#include "utils.h"

gboolean do_events(Imms *imms)
{
    imms->do_events();
    return TRUE;
}

int main(int argc, char **argv)
{
    StackLockFile lock(get_imms_root(".immsd_lock"));
    if (!lock.isok())
    {
        cerr << "IMMSd: another instance already active - exiting." << endl;
        exit(1);
    }

    GMainLoop *loop = g_main_loop_new (NULL, FALSE);

    Imms imms;

    GSource* ts = g_timeout_source_new(1000);
    g_source_attach(ts, NULL);
    g_source_set_callback(ts, (GSourceFunc)do_events, &imms, NULL);

    g_main_loop_run(loop);
    return 0;
}
