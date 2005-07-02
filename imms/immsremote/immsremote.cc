#include <gtk/gtk.h>
#include <glade/glade.h>

#include <list>
#include <string>
#include <set>
#include <vector>

#include "immsutil.h"
#include "appname.h"
#include "sqlite++.h"
#include "giosocket.h"
#include "clientstubbase.h"

#define GLADE_FILE "immsremote/immsremote.glade"
#define MISC_NAME "<Misc>"

using std::string;
using std::list;
using std::set;
using std::vector;

const string AppName = REMOTE_APP;

class RemoteClient;

GtkTreeView *tree;
RemoteClient *client = 0;
GladeXML *mwxml, *cmxml;
int sel_aid = -1;
bool limit_requested = false;
bool refresh_requested = false;

int fold = 10;

enum
{
  COL_ARTIST,
  COL_COUNT,
  COL_SEL,
  COL_AID,
  NUM_COLS
};

class RemoteClient : public GIOSocket
{
public:
    RemoteClient() : connected(false) { }
    bool connect()
    {
        int fd = socket_connect(get_imms_root("socket"));
        if (fd > 0)
        {
            init(fd);
            connected = true;
            write_command("Remote");
            return true;
        }
        LOG(ERROR) << "Connection failed: " << strerror(errno) << endl;
        return false;
    }
    virtual void process_line(const string &line)
    {
        if (line == "Refresh")
        {
            refresh_requested = true;
            return;
        }
        LOG(ERROR) << "Unknown command: " << line << endl;
    }
    virtual void write_command(const string &line)
        { if (isok()) GIOSocket::write(line + "\n"); }
    virtual void connection_lost() { connected = false; }
    bool isok() { return connected; }
private:
    bool connected;
};

extern "C" {
    void on_window_destroy(GtkWindow *window, gpointer user_data);
    void on_refreshitem_activate(GtkButton *button, gpointer user_data);
    void on_folditem_activate(GtkWidget *item, gpointer user_data);
    void on_rename_activate(GtkWidget *item, gpointer user_data);
    void on_markitem_activate(GtkWidget *menuitem, gpointer userdata);
    void on_allitem_activate(GtkWidget *menuitem, gpointer userdata);
    void on_cropitem_activate(GtkWidget *menuitem, gpointer userdata);
    void on_unmarkitem_activate(GtkWidget *menuitem, gpointer userdata);
    gboolean on_maintreeview_button_press_event(GtkWidget *treeview,
            GdkEventButton *event, gpointer userdata);
}

gboolean marked_aid_list(GtkTreeModel *model, GtkTreePath *path,
        GtkTreeIter *iter, gpointer data)
{
    set<int> &aids = *(set<int>*)data;
    gboolean selected;
    int aid;
    gtk_tree_model_get(GTK_TREE_MODEL(model), iter,
            COL_SEL, &selected, COL_AID, &aid, -1);
    if (selected)
        aids.insert(aid);
    return false;
}

void all_aid_list(GtkTreeModel *model, GtkTreePath *path,
        GtkTreeIter *iter, gpointer data)
{
    set<int> &aids = *(set<int>*)data;
    int aid;
    gtk_tree_model_get(GTK_TREE_MODEL(model), iter, COL_AID, &aid, -1);
    aids.insert(aid);
}

gboolean restore_mark(GtkTreeModel *model, GtkTreePath *path,
        GtkTreeIter *iter, gpointer data)
{
    int aid;
    gtk_tree_model_get(GTK_TREE_MODEL(model), iter, COL_AID, &aid, -1);
    set<int> &marked = *(set<int>*)data;
    bool mark = !marked.size() || (marked.find(aid) != marked.end());
    gtk_tree_store_set(GTK_TREE_STORE(model), iter, COL_SEL, mark, -1);
    return false;
}

gboolean restore_select(GtkTreeModel *model, GtkTreePath *path,
        GtkTreeIter *iter, gpointer data)
{
    int aid;
    gtk_tree_model_get(GTK_TREE_MODEL(model), iter, COL_AID, &aid, -1);
    set<int> &selected = *(set<int>*)data;
    if (selected.find(aid) != selected.end())
        gtk_tree_selection_select_iter(
                gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), iter);
    return false;
}

bool mark(const set<int> &marked, int item)
{
    if (!marked.size())
        return true;
    return marked.find(item) != marked.end();
}

struct Artist
{
    Artist() : aid(0), count(0) {}
    int aid, count;
    string name;
};

class ArtistList
{
public:
    ArtistList() { reload(); }
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
    void saveselection()
    {
        // save selection and markings
        marked.clear();
        selected.clear();
        gtk_tree_model_foreach(gtk_tree_view_get_model(tree),
                marked_aid_list, (void*)&marked);
        gtk_tree_selection_selected_foreach(
                gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
                all_aid_list, (void*)&selected);
    }
    void restoreselection()
    {
        gtk_tree_model_foreach(gtk_tree_view_get_model(tree),
                restore_mark, (void*)&marked);
        gtk_tree_model_foreach(gtk_tree_view_get_model(tree),
                restore_select, (void*)&selected);
    }
    void filllist(int threshhold)
    {
        GtkTreeStore *model = GTK_TREE_STORE(gtk_tree_view_get_model(tree));
        GtkTreeIter iter;
        GtkTreeIter fold;

        saveselection();

        g_object_ref(model);
        gtk_tree_view_set_model(GTK_TREE_VIEW(tree), NULL);
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
                -2, GTK_SORT_ASCENDING);

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

            if (i->count <= threshhold)
            {
                gtk_tree_store_append(model, &iter, &fold);
                fold_count += i->count;
            }
            else
                gtk_tree_store_append(model, &iter, 0);

            char *utf8name = g_locale_to_utf8(i->name.c_str(), -1, 0, 0, 0);

            gtk_tree_store_set(model, &iter, COL_ARTIST, utf8name,
                    COL_COUNT, i->count, COL_SEL, true,
                    COL_AID, i->aid, -1);

            g_free(utf8name);
        }

        gtk_tree_store_append(model, &iter, 0);
        gtk_tree_store_set(model, &iter, COL_ARTIST, "<Unknown>",
                COL_COUNT, unknown_count, COL_SEL, true,
                COL_AID, -1, -1);

        if (fold_count)
            gtk_tree_store_set(model, &fold, COL_ARTIST, MISC_NAME,
                    COL_COUNT, fold_count, COL_SEL, true,
                    COL_AID, -2, -1);
        else
            gtk_tree_store_remove(model, &fold);

        gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(model));
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
                COL_ARTIST, GTK_SORT_ASCENDING);
        g_object_unref(model);

        restoreselection();
    }
protected:
    set<int> marked, selected;
    list<Artist> artists;
};

enum RecurseControl {
    RecurseAuto,
    RecurseUp,
    RecurseDown
};

void toggle(GtkTreeIter *iter, int value, RecurseControl r = RecurseAuto)
{
    GtkTreeStore *model = GTK_TREE_STORE(gtk_tree_view_get_model(tree));

    gboolean toggle_item;
    gtk_tree_model_get(GTK_TREE_MODEL(model), iter, COL_SEL, &toggle_item, -1);
    if (value == 1)
        toggle_item = 1;
    else if (value == -1)
        toggle_item = 0;
    else
        toggle_item ^= 1;

    gtk_tree_store_set(GTK_TREE_STORE(model), iter, COL_SEL, toggle_item, -1);

    if ((r == RecurseAuto || r == RecurseDown) &&
            gtk_tree_model_iter_has_child(GTK_TREE_MODEL(model), iter))
    {
        GtkTreeIter child;
        gtk_tree_model_iter_children(GTK_TREE_MODEL(model), &child, iter);
        toggle(&child, toggle_item, RecurseDown);
        while (gtk_tree_model_iter_next(gtk_tree_view_get_model(tree), &child))
            toggle(&child, toggle_item, RecurseDown);
    }

    GtkTreeIter parent;
    if ((r == RecurseAuto || r == RecurseUp) &&
            gtk_tree_model_iter_parent(GTK_TREE_MODEL(model), &parent, iter)
            && !toggle_item)
        toggle(&parent, -1, RecurseUp);

    if (r == RecurseAuto)
        limit_requested = true;
}

void get_aid(GtkTreeModel *model, GtkTreePath *path,
        GtkTreeIter *iter, gpointer data)
{
    gtk_tree_model_get(model, iter, COL_AID, &sel_aid, -1);
}

void selection_action(GtkTreeModel *model, GtkTreePath *path,
        GtkTreeIter *iter, gpointer data)
{
    int value = (int)data;
    toggle(iter, value);
}

void get_name_alternatives(vector<string> &alternatives, int aid)
{
    Q q("SELECT T.artist FROM Library L INNER JOIN Info USING(sid) "
            "INNER JOIN Artists A USING(aid) "
            "INNER JOIN Tags T ON T.uid = L.uid "
            "WHERE A.aid = ? GROUP BY T.artist ORDER BY count(1) DESC;");
    q << aid;

    string name;
    while (q.next())
    {
        q >> name;
        if (name == "")
            continue;
        alternatives.push_back(name);
    }
}

void view_popup_menu(GtkWidget *treeview,
        GdkEventButton *event, gpointer userdata)
{
    GtkWidget *menu = glade_xml_get_widget(cmxml, "contextmenu");

    GtkWidget *renameitem = glade_xml_get_widget(cmxml, "renameitem");
    gtk_widget_set_sensitive(renameitem, false);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(
            GTK_TREE_VIEW(tree));
    if (gtk_tree_selection_count_selected_rows(selection) == 1)
    {
        gtk_tree_selection_selected_foreach(
                gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), get_aid, 0);

        if (sel_aid > -1)
        {
            vector<string> choices;
            get_name_alternatives(choices, sel_aid);
            if (choices.size() > 1)
            {
                GtkWidget* submenu = gtk_menu_new();
                for (size_t i = 0; i < choices.size(); ++i)
                {
                    char *utf8name = g_locale_to_utf8(choices[i].c_str(),
                            -1, 0, 0, 0);
                    GtkWidget *item = gtk_menu_item_new_with_label(utf8name);
                    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
                    g_signal_connect(item, "activate",
                            (GCallback)on_rename_activate, (void*)i);
                    g_free(utf8name);
                }
                gtk_menu_item_set_submenu(GTK_MENU_ITEM(renameitem), submenu);
                gtk_widget_set_sensitive(renameitem, true);
            }
        }
    }

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
            (event != NULL) ? event->button : 0,
            gdk_event_get_time((GdkEvent*)event));
}

void refresh()
{
    ArtistList al;
    al.filllist(fold);
    limit_requested = true;
}

void apply_limit()
{
    set<int> aids;
    gtk_tree_model_foreach(gtk_tree_view_get_model(tree),
            marked_aid_list, (void*)&aids);
    aids.erase(-2);

    try {
        {
            AutoTransaction a(true);
            Q("DELETE FROM DiskMatches;").execute();
            Q("DELETE FROM Selected;").execute();
            a.commit();
        }

        if (!aids.size())
            return;

        bool unknown = false;
        Q q("INSERT INTO Selected VALUES (?);");
        for (set<int>::iterator i = aids.begin(); i != aids.end(); ++i)
        {
            if (*i == -1)
                unknown = true;
            else
            {
                q << *i;
                q.execute();
            }
        }

        {
            string where_clause = "WHERE (A.aid IN Selected)";
            if (unknown)
                where_clause += " OR uid = -1 OR aid ISNULL";

            AutoTransaction a(true);
            Q q("INSERT INTO DiskMatches SELECT DISTINCT uid "
                    "FROM DiskPlaylist INNER JOIN Library USING(uid) "
                    "INNER JOIN Info USING(sid) "
                    "INNER JOIN Artists A USING(aid) " + where_clause + ";");
            q.execute();
            a.commit();
        }
    }
    WARNIFFAILED();

    client->write_command("Sync");
}

void rename(int aid, unsigned alternative)
{
    vector<string> choices;
    get_name_alternatives(choices, aid);

    if (choices.size() <= alternative)
    {
        LOG(ERROR) << "Invalid alternative!" << endl;
        return;
    }

    try {
        AutoTransaction a(true);
        Q q("UPDATE Artists SET readable = ?, trust = 500 WHERE aid = ?;");
        q << choices[alternative] << aid;
        q.execute();
        a.commit();
    }
    WARNIFFAILED();
}

void on_rename_activate(GtkWidget *item, gpointer user_data)
{
    rename(sel_aid, (unsigned)user_data);
    refresh();
}

void on_cropitem_activate(GtkWidget *menuitem, gpointer userdata)
{
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(gtk_tree_view_get_model(tree), &iter))
    {
        toggle(&iter, -1);
        while (gtk_tree_model_iter_next(gtk_tree_view_get_model(tree), &iter))
            toggle(&iter, -1);
    }

    on_markitem_activate(0, 0);
}

void on_allitem_activate(GtkWidget *menuitem, gpointer userdata)
{
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(gtk_tree_view_get_model(tree), &iter))
    {
        toggle(&iter, 1);
        while (gtk_tree_model_iter_next(gtk_tree_view_get_model(tree), &iter))
            toggle(&iter, 1);
    }
}

void on_markitem_activate(GtkWidget *menuitem, gpointer userdata)
{
    gtk_tree_selection_selected_foreach(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
            selection_action, (void*)1);
}

void on_unmarkitem_activate(GtkWidget *menuitem, gpointer userdata)
{
    gtk_tree_selection_selected_foreach(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
            selection_action, (void*)-1);
}

gboolean on_maintreeview_button_press_event(GtkWidget *treeview,
            GdkEventButton *event, gpointer userdata)
{
    if (event->type != GDK_BUTTON_PRESS || event->button != 3)
        return FALSE;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(
            GTK_TREE_VIEW(tree));
    if (gtk_tree_selection_count_selected_rows(selection) < 1)
    {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree),
                    (gint)event->x, (gint)event->y, &path, 0, 0, 0))
        {
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

void on_folditem_activate(GtkWidget *item, gpointer user_data)
{
    fold = (int)user_data;
    refresh();
}

void on_refreshitem_activate(GtkButton *button, gpointer user_data)
{
    refresh();
}

void item_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);

    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, path);
    toggle(&iter, 0);
    gtk_tree_path_free(path);
}

gboolean limit_action(void *unused)
{
    if (limit_requested)
    {
        apply_limit();
        limit_requested = false;
    }
    if (refresh_requested)
    {
        refresh();
        refresh_requested = false;
    }
    if (client && !client->isok())
        client->connect();
    return TRUE;
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    glade_init();

    SQLDatabaseConnection db(get_imms_root("imms2.db"));
    try {
        Q("CREATE TEMP TABLE Selected "
                "('aid' INTEGER UNIQUE NOT NULL);").execute();
    }
    WARNIFFAILED();

    mwxml = glade_xml_new(GLADE_FILE, "mainwindow", NULL);
    cmxml = glade_xml_new(GLADE_FILE, "contextmenu", NULL);

    if (!mwxml || !cmxml)
    {
        g_warning("Problem while loading the .glade file");
        return 1;
    }

    glade_xml_signal_autoconnect(mwxml);
    glade_xml_signal_autoconnect(cmxml);

    tree = GTK_TREE_VIEW(glade_xml_get_widget(mwxml, "maintreeview"));

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
            G_TYPE_STRING, G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_INT);
    gtk_tree_view_set_model(tree, GTK_TREE_MODEL(model));
    g_signal_connect(renderer, "toggled", G_CALLBACK(item_toggled), model);
    g_object_unref(model);

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(
                GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);

    GtkCheckMenuItem *mi;
    mi = GTK_CHECK_MENU_ITEM(glade_xml_get_widget(cmxml, "1fold"));
    g_signal_connect(mi, "activate",
            (GCallback)on_folditem_activate, (void*)1);
    mi = GTK_CHECK_MENU_ITEM(glade_xml_get_widget(cmxml, "5fold"));
    g_signal_connect(mi, "activate",
            (GCallback)on_folditem_activate, (void*)5);
    mi = GTK_CHECK_MENU_ITEM(glade_xml_get_widget(cmxml, "10fold"));
    g_signal_connect(mi, "activate",
            (GCallback)on_folditem_activate, (void*)10);
    mi = GTK_CHECK_MENU_ITEM(glade_xml_get_widget(cmxml, "25fold"));
    g_signal_connect(mi, "activate",
            (GCallback)on_folditem_activate, (void*)25);

    RemoteClient remoteclient;
    client = &remoteclient;

    GSource* ts = g_timeout_source_new(500);
    g_source_attach(ts, NULL);
    g_source_set_callback(ts, (GSourceFunc)limit_action, NULL, NULL);

    gtk_main();

    return 0;
}
