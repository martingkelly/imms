#include <gtk/gtk.h>
#include <glade/glade.h>

#include <list>
#include <string>

#include "immsutil.h"
#include "sqlite++.h"

using std::string;
using std::list;

GtkTreeView *tree;
GladeXML *xml;

enum
{
  COL_ARTIST,
  COL_COUNT,
  COL_SEL,
  NUM_COLS
};

struct Artist
{
    Artist() : aid(0), count(0) {}
    int aid, count;
    string name;
};

class ArtistList
{
public:
    ArtistList()
    {
        reload();
    }
    void reload()
    {
        try
        {
            Q q("SELECT aid, readable, count(1) FROM DiskPlaylist "
                    "NATURAL JOIN Library NATURAL JOIN Info "
                    "NATURAL JOIN Artists GROUP BY aid;");

            while (q.next())
            {
                Artist a;
                q >> a.aid >> a.name >> a.count;
                artists.push_back(a);
            }
        }
        WARNIFFAILED();
    }
    void filllist(int threshhold)
    {
        GtkTreeStore *model = GTK_TREE_STORE(gtk_tree_view_get_model(tree));
        GtkTreeIter iter;
        GtkTreeIter fold;

        g_object_ref(model);
        gtk_tree_view_set_model(GTK_TREE_VIEW(tree), NULL);

        gtk_tree_store_clear(model);

        gtk_tree_store_append(model, &fold, 0);

        int unknown_count = 0;
        int fold_count = 0;

        for (list<Artist>::iterator i = artists.begin();
                i != artists.end(); ++i)
        {
            if (i->name == "")
            {
                unknown_count += i->count;
                continue;
            }

            if (i->count < threshhold)
            {
                gtk_tree_store_append(model, &iter, &fold);
                fold_count += i->count;
            }
            else
                gtk_tree_store_append(model, &iter, 0);

            char *utf8name = g_locale_to_utf8(i->name.c_str(), -1, 0, 0, 0);

            gtk_tree_store_set(model, &iter, COL_ARTIST, utf8name,
                    COL_COUNT, i->count, -1);

            g_free(utf8name);
        }

        gtk_tree_store_append(model, &iter, 0);
        gtk_tree_store_set(model, &iter,
                COL_ARTIST, "<Unknown>",
                COL_COUNT, unknown_count, -1);

        if (fold_count)
        {
            gtk_tree_store_set(model, &fold,
                    COL_ARTIST, "<Misc>",
                    COL_COUNT, fold_count, -1);
        }
        else
            gtk_tree_store_remove(model, &fold);

        gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(model));
        g_object_unref(model);
    }
    void reset_aids()
    {
        unknown = false;
        aids.clear();
    }
    void load_aids(const string &artist)
    {
        if (artist == "<Unknown>")
        {
            unknown = true;
            return;
        }
    }
    void finish_aids()
    {

    }
protected:
    list<Artist> artists;
    vector<int> aids;
    bool unknown;
};

void selection_build(GtkTreeModel *model, GtkTreePath *path,
        GtkTreeIter *iter, gpointer data)
{
    char *artist;
    gtk_tree_model_get(model, iter, COL_ARTIST, &artist, -1);
    cerr << artist << " is selected!" << endl;
    g_free(artist);
}


void view_popup_menu(GtkWidget *treeview,
        GdkEventButton *event, gpointer userdata)
{
    GtkWidget *menu = glade_xml_get_widget(xml, "contextmenu");

    if (!menu)
        cerr << "Whaa!!" << endl;

    /*
    g_signal_connect(menuitem, "activate",
            (GCallback) view_popup_menu_onDoSomething, treeview);
            */

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
            (event != NULL) ? event->button : 0,
            gdk_event_get_time((GdkEvent*)event));
}

extern "C" {
    void on_window_destroy(GtkWindow *window, gpointer user_data);
    void on_refreshbutton_clicked(GtkButton *button, gpointer user_data);
    void on_limitbutton_clicked(GtkButton *button, gpointer user_data);
    gboolean on_maintreeview_button_press_event(GtkWidget *treeview,
            GdkEventButton *event, gpointer userdata);
}

gboolean on_maintreeview_button_press_event(GtkWidget *treeview,
            GdkEventButton *event, gpointer userdata)
{
    if (event->type != GDK_BUTTON_PRESS || event->button != 3)
        return FALSE;

    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    if (gtk_tree_selection_count_selected_rows(selection) <= 1)
    {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                    (gint)event->x, (gint)event->y, &path, 0, 0, 0))
        {
            gtk_tree_selection_unselect_all(selection);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
        }
    }

    view_popup_menu(treeview, event, userdata);
    return TRUE;
}

void on_window_destroy(GtkWindow *window, gpointer user_data)
{
    gtk_main_quit();
}

void on_refreshbutton_clicked(GtkButton *button, gpointer user_data)
{
    ArtistList al;
    GtkSpinButton *foldspin =
        GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "foldspin"));
    al.filllist(gtk_spin_button_get_value_as_int(foldspin));
}

void on_limitbutton_clicked(GtkButton *button, gpointer user_data)
{
    gtk_tree_selection_selected_foreach(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
                selection_build, 0);
}

void item_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);

    GtkTreeIter iter;
    gboolean toggle_item;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, COL_SEL, &toggle_item, -1);

    toggle_item ^= 1;

    gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COL_SEL, toggle_item, -1);

    gtk_tree_path_free(path);
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    glade_init();

    SQLDatabaseConnection db(get_imms_root("imms2.db"));

    xml = glade_xml_new("immsremote/immsremote.glade", "mainwindow", NULL);

    if (!xml)
    {
        g_warning("Problem while loading the .glade file");
        return 1;
    }

    glade_xml_signal_autoconnect(xml);

    tree = GTK_TREE_VIEW(glade_xml_get_widget(xml, "maintreeview"));

    GtkTreeViewColumn *column;

    column = gtk_tree_view_column_new_with_attributes(
            "Name", gtk_cell_renderer_text_new(), "text", COL_ARTIST, NULL);
    gtk_tree_view_column_set_sort_column_id(column, COL_ARTIST);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    column = gtk_tree_view_column_new_with_attributes(
            "Count", gtk_cell_renderer_text_new(), "text", COL_COUNT, NULL);
    gtk_tree_view_column_set_sort_column_id(column, COL_COUNT);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
    g_object_set(renderer, "xalign", 0.0, NULL);

    column = gtk_tree_view_column_new_with_attributes(
            "Selected", renderer, "active", COL_SEL, NULL);

    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(column, 50);
    gtk_tree_view_column_set_clickable(column, true);
    gtk_tree_view_column_set_sort_column_id(column, COL_SEL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    GtkTreeStore *model = gtk_tree_store_new(NUM_COLS,
            G_TYPE_STRING, G_TYPE_UINT, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(tree, GTK_TREE_MODEL(model));
    g_signal_connect(renderer, "toggled", G_CALLBACK(item_toggled), model);
    g_object_unref(model);

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(
                GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);

    gtk_main();

    return 0;
}
