#include <dbus/dbus.h>
#include <glib.h>

DBusHandlerResult dispatch_wrapper(DBusConnection *con,
        DBusMessage *message, void *userdata);

struct SourceArgs
{
    guint sourceid;
    GList *watch_fds;
    DBusConnection *con;
};

typedef struct
{
    int refcount;

    GPollFD poll_fd;
    DBusWatch *watch;

    unsigned int removed : 1;
} WatchFD;

static WatchFD *watch_fd_new(void)
{
    WatchFD *watch_fd;

    watch_fd = g_new0 (WatchFD, 1);
    watch_fd->refcount = 1;

    return watch_fd;
}

static WatchFD *watch_fd_ref(WatchFD *watch_fd)
{
    watch_fd->refcount += 1;

    return watch_fd;
}

static void watch_fd_unref(WatchFD *watch_fd)
{
    watch_fd->refcount -= 1;

    if (watch_fd->refcount == 0)
    {
        g_assert (watch_fd->removed);
        g_free (watch_fd);
    }
}

static gboolean glib1_connection_prepare(gpointer srcdata,
        GTimeVal *current_time, gint *timeout, gpointer userdata)
{
    DBusConnection *con = ((SourceArgs*)srcdata)->con;
    return (dbus_connection_get_dispatch_status(con)
            == DBUS_DISPATCH_DATA_REMAINS);
}

static gboolean glib1_connection_check(gpointer srcdata,
        GTimeVal *current_time, gpointer userdata)
{
    GList *list = ((SourceArgs*)srcdata)->watch_fds;

    while (list)
    {
        WatchFD *watch_fd = (WatchFD*)list->data;

        if (watch_fd->poll_fd.revents != 0)
            return TRUE;

        list = list->next;
    }

    return FALSE;
}

static gboolean dbus_gsource_dispatch(SourceArgs *source)
{
    GList *copy, *list;

    /* Make a copy of the list and ref all WatchFDs */
    copy = g_list_copy(source->watch_fds);
    g_list_foreach (copy, (GFunc)watch_fd_ref, NULL);

    list = copy;
    while(list)
    {
        WatchFD *watch_fd = (WatchFD*)list->data;

        if (!watch_fd->removed && watch_fd->poll_fd.revents != 0)
        {
            guint condition = 0;

            if (watch_fd->poll_fd.revents & G_IO_IN)
                condition |= DBUS_WATCH_READABLE;
            if (watch_fd->poll_fd.revents & G_IO_OUT)
                condition |= DBUS_WATCH_WRITABLE;
            if (watch_fd->poll_fd.revents & G_IO_ERR)
                condition |= DBUS_WATCH_ERROR;
            if (watch_fd->poll_fd.revents & G_IO_HUP)
                condition |= DBUS_WATCH_HANGUP;

            dbus_watch_handle(watch_fd->watch, condition);
        }

        list = list->next;
    }

    g_list_foreach(copy, (GFunc)watch_fd_unref, NULL);   
    g_list_free(copy);   

    return TRUE;
}

gboolean glib1_connection_dispatch(gpointer srcdata,
        GTimeVal *current_time, gpointer userdata)
{
    DBusConnection *con = ((SourceArgs*)srcdata)->con;

    dbus_connection_ref(con);

    dbus_gsource_dispatch((SourceArgs*)srcdata);

    while (dbus_connection_dispatch(con) == DBUS_DISPATCH_DATA_REMAINS);

    dbus_connection_unref(con);
    return TRUE;
}

static void free_args(SourceArgs *args)
{
    g_source_remove(args->sourceid);
    g_free(args);
}

static dbus_bool_t add_watch(DBusWatch *watch, gpointer data)
{
    WatchFD *watch_fd;
    SourceArgs *args = (SourceArgs*)data;
    guint flags;

    if (!dbus_watch_get_enabled(watch))
        return TRUE;

    watch_fd = watch_fd_new();
    watch_fd->poll_fd.fd = dbus_watch_get_fd(watch);
    watch_fd->poll_fd.events = 0;
    flags = dbus_watch_get_flags(watch);
    dbus_watch_set_data(watch, watch_fd, (DBusFreeFunction)watch_fd_unref);

    if (flags & DBUS_WATCH_READABLE)
        watch_fd->poll_fd.events |= G_IO_IN;
    if (flags & DBUS_WATCH_WRITABLE)
        watch_fd->poll_fd.events |= G_IO_OUT;
    watch_fd->poll_fd.events |= G_IO_ERR | G_IO_HUP;

    watch_fd->watch = watch;

    g_main_add_poll(&watch_fd->poll_fd, G_PRIORITY_DEFAULT_IDLE);

    args->watch_fds = g_list_prepend(args->watch_fds, watch_fd);

    return TRUE;
}

static void remove_watch(DBusWatch *watch, gpointer data)
{
    SourceArgs *args = (SourceArgs*)data;
    WatchFD *watch_fd = (WatchFD*)dbus_watch_get_data(watch);

    if (watch_fd == NULL)
        return;

    watch_fd->removed = TRUE;
    watch_fd->watch = NULL;  

    args->watch_fds = g_list_remove(args->watch_fds, watch_fd);

    g_main_remove_poll(&watch_fd->poll_fd);

    dbus_watch_set_data(watch, NULL, NULL);
}

static void watch_toggled(DBusWatch *watch, void *data)
{
    if (dbus_watch_get_enabled (watch))
        add_watch (watch, data);
    else
        remove_watch (watch, data);
}

static gboolean timeout_handler(gpointer data)
{
    DBusTimeout *timeout = (DBusTimeout*)data;

    dbus_timeout_handle(timeout);

    return TRUE;
}

static dbus_bool_t add_timeout(DBusTimeout *timeout, void *data)
{
    if (!dbus_timeout_get_enabled (timeout))
        return TRUE;

    guint tid = g_timeout_add(dbus_timeout_get_interval(timeout),
            timeout_handler, NULL);

    dbus_timeout_set_data(timeout, GUINT_TO_POINTER(tid), NULL);

    return TRUE;
}

static void remove_timeout(DBusTimeout *timeout, void *data)
{
    guint timeout_tag = GPOINTER_TO_UINT(dbus_timeout_get_data(timeout));

    if (timeout_tag != 0)
        g_source_remove(timeout_tag);
}

static void timeout_toggled(DBusTimeout *timeout, void *data)
{
    if (dbus_timeout_get_enabled(timeout))
        add_timeout(timeout, data);
    else
        remove_timeout(timeout, data);
}

static void wakeup_main(void *data) { }

GSourceFuncs funcs = {
    glib1_connection_prepare,
    glib1_connection_check,
    glib1_connection_dispatch,
    NULL                  
};

void setup_connection_with_main_loop(DBusConnection *con, void *userdata)
{
    static dbus_int32_t connection_slot = -1;

    SourceArgs *args = (SourceArgs*)g_malloc(sizeof(SourceArgs));
    args->watch_fds = 0;
    args->con = con;

    dbus_connection_allocate_data_slot(&connection_slot);
    dbus_connection_set_data(con, connection_slot, args,
            (DBusFreeFunction)free_args);

    dbus_connection_set_watch_functions(con,
            add_watch, remove_watch, watch_toggled, args, NULL);

    dbus_connection_set_timeout_functions(con,
            add_timeout, remove_timeout, timeout_toggled, args, NULL);

    dbus_connection_set_wakeup_main_function(con,
            wakeup_main, args, NULL);

    args->sourceid = g_source_add(G_PRIORITY_DEFAULT, false, &funcs, args, 0, 0);
}
