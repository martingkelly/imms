#include <gtk/gtk.h>

#include <xmms/configfile.h>
#include <xmms/util.h>
#include <xmms/plugin.h>

#include "immsconf.h"
#include "plugin.h"

char *ch_email = NULL;
int use_xidle = 1;
int use_sloppy = 0;
int poll_tag = 0;

GtkWidget *configure_win = NULL, *about_win = NULL,
    *xidle_button = NULL, *sloppy_button = NULL;

gint poll_func(gpointer unused)
{
    imms_poll();
    return TRUE;
}

void read_config(void)
{
    ConfigFile *cfgfile;

    g_free(ch_email);
    ch_email = NULL;

    if ((cfgfile = xmms_cfg_open_default_file()) != NULL)
    {
        xmms_cfg_read_string(cfgfile, "imms", "email", &ch_email);
        xmms_cfg_read_int(cfgfile, "imms", "xidle", &use_xidle);
        xmms_cfg_read_int(cfgfile, "imms", "sloppy", &use_sloppy);
        xmms_cfg_free(cfgfile);
    }
}

void init(void)
{
    imms_init();
    read_config();
    imms_setup(ch_email, use_xidle, use_sloppy);
    poll_tag = gtk_timeout_add(30, poll_func, NULL);
}

void cleanup(void)
{
    imms_cleanup();

    if (poll_tag)
        gtk_timeout_remove(poll_tag);

    poll_tag = 0;
}

void configure_ok_cb(gpointer data)
{
    ConfigFile *cfgfile = xmms_cfg_open_default_file();

    use_xidle = !!GTK_TOGGLE_BUTTON(xidle_button)->active;
    use_sloppy = !!GTK_TOGGLE_BUTTON(sloppy_button)->active;

    xmms_cfg_write_int(cfgfile, "imms", "xidle", use_xidle);
    xmms_cfg_write_int(cfgfile, "imms", "sloppy", use_sloppy);
    xmms_cfg_write_default_file(cfgfile);

    xmms_cfg_free(cfgfile);

    imms_setup(ch_email, use_xidle, use_sloppy);
    gtk_widget_destroy(configure_win);
}  

void configure(void)
{
    /* Main */
    GtkWidget *configure_vbox;
    /* XIdle */
    GtkWidget *xidle_hbox, *xidle_vbox, *xidle_frame, *xidle_desc; 
    /* Crossfade */
    GtkWidget *sloppy_hbox, *sloppy_vbox, *sloppy_frame, *sloppy_desc; 
    /* Buttons */
    GtkWidget *configure_bbox, *configure_ok, *configure_cancel;

    if (configure_win)
        return;

    read_config();

    configure_win = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_signal_connect(GTK_OBJECT(configure_win), "destroy",
            GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure_win);
    gtk_window_set_title(GTK_WINDOW(configure_win), "IMMS Configuration");

    gtk_container_set_border_width(GTK_CONTAINER(configure_win), 10);

    configure_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(configure_win), configure_vbox);

    /* Xidle */
    xidle_frame = gtk_frame_new("Idleness");
    gtk_box_pack_start(GTK_BOX(configure_vbox), xidle_frame, FALSE, FALSE, 0);
    xidle_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(xidle_vbox), 5);
    gtk_container_add(GTK_CONTAINER(xidle_frame), xidle_vbox);

    xidle_desc = gtk_label_new(
            "Disable if you use XMMS on a dedicated machine");
    gtk_label_set_justify(GTK_LABEL(xidle_desc), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(xidle_desc), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(xidle_vbox), xidle_desc, FALSE, FALSE, 0);
    gtk_widget_show(xidle_desc);

    xidle_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(xidle_vbox), xidle_hbox, FALSE, FALSE, 0);

    xidle_button =
        gtk_check_button_new_with_label("Use X idleness statistics");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xidle_button), use_xidle);
    gtk_box_pack_start(GTK_BOX(xidle_hbox), xidle_button, FALSE, FALSE, 0);

    gtk_widget_show(xidle_frame);
    gtk_widget_show(xidle_vbox);
    gtk_widget_show(xidle_button);
    gtk_widget_show(xidle_hbox);

    /* Crossfade */
    sloppy_frame = gtk_frame_new("Skip detection");
    gtk_box_pack_start(GTK_BOX(configure_vbox), sloppy_frame, FALSE, FALSE, 0);
    sloppy_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(sloppy_vbox), 5);
    gtk_container_add(GTK_CONTAINER(sloppy_frame), sloppy_vbox);

    sloppy_desc = gtk_label_new(
            "Enable this if you use XMMS Crossfade plugin, "
            "or experience misdetected song skips.");
    gtk_label_set_justify(GTK_LABEL(sloppy_desc), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(sloppy_desc), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(sloppy_vbox), sloppy_desc, FALSE, FALSE, 0);
    gtk_widget_show(sloppy_desc);

    sloppy_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(sloppy_vbox), sloppy_hbox, FALSE, FALSE, 0);

    sloppy_button =
        gtk_check_button_new_with_label("Use sloppy skip detection");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sloppy_button), use_sloppy);
    gtk_box_pack_start(GTK_BOX(sloppy_hbox), sloppy_button, FALSE, FALSE, 0);

    gtk_widget_show(sloppy_frame);
    gtk_widget_show(sloppy_vbox);
    gtk_widget_show(sloppy_button);
    gtk_widget_show(sloppy_hbox);

    /* Buttons */
    configure_bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(configure_bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(configure_bbox), 5);
    gtk_box_pack_start(GTK_BOX(configure_vbox), configure_bbox, FALSE, FALSE, 0);

    configure_ok = gtk_button_new_with_label("Ok");
    gtk_signal_connect(GTK_OBJECT(configure_ok), "clicked",
            GTK_SIGNAL_FUNC(configure_ok_cb), NULL);
    GTK_WIDGET_SET_FLAGS(configure_ok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(configure_bbox), configure_ok, TRUE, TRUE, 0);
    gtk_widget_show(configure_ok);
    gtk_widget_grab_default(configure_ok);

    configure_cancel = gtk_button_new_with_label("Cancel");
    gtk_signal_connect_object(GTK_OBJECT(configure_cancel), "clicked",
            GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(configure_win));
    GTK_WIDGET_SET_FLAGS(configure_cancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(configure_bbox), configure_cancel, TRUE, TRUE, 0);
    gtk_widget_show(configure_cancel);
    gtk_widget_show(configure_bbox);
    gtk_widget_show(configure_vbox);
    gtk_widget_show(configure_win);
}

void about(void)
{
    if (about_win)
        return;

    about_win = xmms_show_message(
            "About IMMS",
            PACKAGE_STRING "\n\n"
            "Intelligent Multimedia Management System" "\n\n"
            "IMMS is an intelligent playlist plug-in for XMMS" "\n"
            "that tracks your listening patterns" "\n"
            "and dynamically adapts to your taste." "\n\n"
            "It is incredibly unobtrusive and easy to use" "\n"
            "as it requires no direct user interaction." "\n\n"
            "For more information please visit" "\n"
            "http://www.luminal.org/wiki/index.php/IMMS" "\n\n"
            "Written by" "\n"
            "Michael \"mag\" Grigoriev <mag@luminal.org>",
            "Dismiss", FALSE, NULL, NULL);

    gtk_signal_connect(GTK_OBJECT(about_win), "destroy",
            GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about_win);
}

GeneralPlugin imms_gp =
{
    NULL,                   /* handle */
    NULL,                   /* filename */
    -1,                     /* xmms_session */
    NULL,                   /* Description */
    init,
    about,
    configure,
    cleanup,
};

GeneralPlugin *get_gplugin_info(void)
{
    imms_gp.description = PACKAGE_STRING;
    return &imms_gp;
}
