/*
  IMMS: Intelligent Multimedia Management System
  Copyright (C) 2001-2009 Michael Grigoriev

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
#include <gtk/gtk.h>

#include <xmms/configfile.h>
#include <xmms/util.h>
#include <xmms/plugin.h>

#include "immsconf.h"
#include "cplugin.h"


int use_xidle = 1;
int poll_tag = 0;

GtkWidget *configure_win = NULL, *about_win = NULL, *xidle_button = NULL;

gint poll_func(gpointer unused)
{
    imms_poll();
    return TRUE;
}

void read_config(void)
{
    ConfigFile *cfgfile;

    if ((cfgfile = xmms_cfg_open_default_file()) != NULL)
    {
        xmms_cfg_read_int(cfgfile, "imms", "xidle", &use_xidle);
        xmms_cfg_free(cfgfile);
    }
}

void init(void)
{
    imms_init();
    read_config();
    imms_setup(use_xidle);
    poll_tag = gtk_timeout_add(200, poll_func, NULL);
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

    xmms_cfg_write_int(cfgfile, "imms", "xidle", use_xidle);
    xmms_cfg_write_default_file(cfgfile);

    xmms_cfg_free(cfgfile);

    imms_setup(use_xidle);
    gtk_widget_destroy(configure_win);
}  

#define ADD_CONFIG_CHECKBOX(pref, title, label, descr)                          \
    pref##_frame = gtk_frame_new(title);                                        \
    gtk_box_pack_start(GTK_BOX(configure_vbox), pref##_frame, FALSE, FALSE, 0); \
    pref##_vbox = gtk_vbox_new(FALSE, 10);                                      \
    gtk_container_set_border_width(GTK_CONTAINER(pref##_vbox), 5);              \
    gtk_container_add(GTK_CONTAINER(pref##_frame), pref##_vbox);                \
                                                                                \
    pref##_desc = gtk_label_new(label);                                         \
                                                                                \
    gtk_label_set_line_wrap(GTK_LABEL(pref##_desc), TRUE);                      \
    gtk_label_set_justify(GTK_LABEL(pref##_desc), GTK_JUSTIFY_LEFT);            \
    gtk_misc_set_alignment(GTK_MISC(pref##_desc), 0, 0.5);                      \
    gtk_box_pack_start(GTK_BOX(pref##_vbox), pref##_desc, FALSE, FALSE, 0);     \
    gtk_widget_show(pref##_desc);                                               \
                                                                                \
    pref##_hbox = gtk_hbox_new(FALSE, 5);                                       \
    gtk_box_pack_start(GTK_BOX(pref##_vbox), pref##_hbox, FALSE, FALSE, 0);     \
                                                                                \
    pref##_button = gtk_check_button_new_with_label(descr);                     \
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pref##_button), use_##pref); \
    gtk_box_pack_start(GTK_BOX(pref##_hbox), pref##_button, FALSE, FALSE, 0);   \
                                                                                \
    gtk_widget_show(pref##_frame);                                              \
    gtk_widget_show(pref##_vbox);                                               \
    gtk_widget_show(pref##_button);                                             \
    gtk_widget_show(pref##_hbox);

void configure(void)
{
    GtkWidget *configure_vbox;
    GtkWidget *xidle_hbox, *xidle_vbox, *xidle_frame, *xidle_desc; 
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

    ADD_CONFIG_CHECKBOX(xidle, "Idleness", 
            "Disable this option if you use XMMS on a dedicated machine",
            "Use X idleness statistics");

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
