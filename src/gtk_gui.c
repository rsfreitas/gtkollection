
/*
 * Description: Gtk UI implementation.
 */

#include <stdlib.h>
#include <string.h>
#include <libintl.h>

#include "gtkollection.h"

#define MSG_BLOCK_APP               1
#define MSG_GIVE_CHOICE             2

#define gettext_noop(String)        String

static GtkWidget *__main_window;
static GtkWidget *__notebook;
static GtkWidget *__vpane;
static GtkWidget *__hpane;
static GList *__db_collection = NULL;
static GList *__dlg_data = NULL;
static struct app_settings *__settings;

static void quit(GtkWidget *w, gpointer data);
static void about(GtkWidget *w, gpointer data);
static void add_collection(GtkWidget *w, gpointer data);
static void change_collection(GtkWidget *w, gpointer data);
static void del_collection(GtkWidget *w, gpointer data);

static GtkActionEntry __menu_items[] = {
    { "MainMenuAction",       GTK_STOCK_FILE,   gettext_noop("_Main"),       NULL, NULL, NULL },
    { "CollectionMenuAction", GTK_STOCK_FILE,   gettext_noop("_Collection"), NULL, NULL, NULL },
    { "HelpMenuAction",       GTK_STOCK_HELP,   gettext_noop("_Help"),       NULL, NULL, NULL },
    { "Exit",                 GTK_STOCK_QUIT,   gettext_noop("E_xit"),       NULL, NULL, G_CALLBACK(quit) },
    { "AddCollection",        GTK_STOCK_ADD,    gettext_noop("_Add"),        NULL, NULL, G_CALLBACK(add_collection) },
    { "ChangeCollection",     GTK_STOCK_EDIT,   gettext_noop("_Change"),     NULL, NULL, G_CALLBACK(change_collection) },
    { "DeleteCollection",     GTK_STOCK_DELETE, gettext_noop("_Delete"),     NULL, NULL, G_CALLBACK(del_collection) },
    { "About",                GTK_STOCK_ABOUT,  gettext_noop("_About"),      NULL, NULL, G_CALLBACK(about) },
};

static int __nmenu_items = sizeof(__menu_items) / sizeof(__menu_items[0]);

static const char *__ui_string = "\
    <ui> \
        <menubar name=\"MainMenu\"> \
            <menu name=\"Main\" action=\"MainMenuAction\" > \
                <menuitem name=\"Exit\" action=\"Exit\" /> \
            </menu> \
            <menu name=\"Collection\" action=\"CollectionMenuAction\" > \
                <menuitem name=\"Add\" action=\"AddCollection\" /> \
                <menuitem name=\"Change\" action=\"ChangeCollection\" /> \
                <menuitem name=\"Delete\" action=\"DeleteCollection\" /> \
            </menu> \
            <menu name=\"Help\" action=\"HelpMenuAction\" > \
                <menuitem name=\"About\" action=\"About\" /> \
            </menu> \
        </menubar> \
    </ui> \
";

static void get_window_settings(void)
{
    if (__settings->maximized == FALSE)
        gtk_window_get_size(GTK_WINDOW(__main_window), &__settings->wnd_width,
                            &__settings->wnd_height);

    gtk_window_get_position(GTK_WINDOW(__main_window), &__settings->pos_x,
                            &__settings->pos_y);
}

static char *ui_has_unsaved_data(GList *dlg_data_list)
{
    GList *l;
    struct dlg_data *dlg_data;

    for (l = g_list_first(dlg_data_list); l; l = l->next) {
        dlg_data = (struct dlg_data *)l->data;

        if (gtk_widget_get_sensitive(dlg_data->bt_save))
            return dlg_data->c->screen_name;
    }

    return NULL;
}

static int check_unsaved_data(int dlg_type)
{
    char *collection;

    collection = ui_has_unsaved_data(__dlg_data);

    if (collection != NULL) {
        if (dlg_type == MSG_BLOCK_APP) {
            display_msg(GTK_MESSAGE_WARNING, gettext("Attention"),
                        gettext("There are unsaved data in the '%s' collection. "
                                "Save them before using this option."),
                                collection);

            return 0;
        } else {
            if (choose_msg(gettext("Attention"),
                           gettext("There are unsaved data in the '%s' collection. "
                                   "Do you want to continue without save them?"),
                                   collection))
            {
                return 1;
            } else
                return 0;
        }
    }

    return 1;
}

static struct db_collection *search_db_collection_list(const char *name, int *idx)
{
    GList *l;
    int i=-1;
    struct db_collection *c;

    for (l = g_list_first(__db_collection), i = 0; l; l = l->next, i++) {
        c = (struct db_collection *)l->data;

        if (!strcmp((char *)c->name, name)) {
            *idx = i;
            return c;
        }
    }

    *idx = -1;
    return NULL;
}

static struct dlg_data *search_dlg_data_list(struct db_collection *c)
{
    GList *l;
    struct dlg_data *dlg;

    for (l = g_list_first(__dlg_data); l; l = l->next) {
        dlg = (struct dlg_data *)l->data;

        if (!strcmp(dlg->c->name, c->name))
            return dlg;
    }

    return NULL;
}

static void ui_remove_notebook(const char *db_name, GtkWidget *notebook,
    int remove_database)
{
    struct db_collection *c;
    struct dlg_data *dlg;
    int index=-1;

    c = search_db_collection_list(db_name, &index);

    if (c == NULL)
        return;

    /* remove from database */
    if (remove_database == TRUE)
        db_delete_collection(db_name);

    /* remove from internal dlg_data list */
    dlg = search_dlg_data_list(c);

    if (dlg != NULL)
        __dlg_data = g_list_remove(__dlg_data, dlg);

    /* remove from internal list */
    __db_collection = g_list_remove(__db_collection, c);

    /* update user screen */
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), index);
}

static void ui_create_notebook(struct db_collection *c, gpointer user_data)
{
    struct dlg_data *dlg;
    int *index;
    char label[128]={0};

    if (user_data != NULL) {
        index = (int *)user_data;
        __db_collection = g_list_insert(__db_collection, c, *index);
    }

    dlg = collection_widget(c, __notebook);
    sprintf(label, gettext("%s - %d Items"), c->screen_name, c->n_entries);

    if (user_data != NULL) {
        gtk_notebook_insert_page(GTK_NOTEBOOK(__notebook), dlg->page,
                                 gtk_label_new(label), *index);
    } else
        gtk_notebook_append_page(GTK_NOTEBOOK(__notebook), dlg->page,
                                 gtk_label_new(label));

    gtk_widget_show_all(__notebook);

    if (user_data != NULL)
        __dlg_data = g_list_insert(__dlg_data, dlg, *index);
    else
        __dlg_data = g_list_append(__dlg_data, dlg);
}

static void quit(GtkWidget *w __attribute__((unused)),
    gpointer data __attribute__((unused)))
{
    if (!check_unsaved_data(MSG_GIVE_CHOICE))
        return;

    get_window_settings();
    gtk_main_quit();
}

static void about(GtkWidget *w __attribute__((unused)),
    gpointer data __attribute__((unused)))
{
    char *license=NULL;

    license = load_license_file();

    gtk_show_about_dialog(GTK_WINDOW(ui_get_mainwindow()),
                          "program-name", "gtkollection",
                          "comments", "Software to manage collections",
                          "license", (license != NULL ? license : "GPLv2"),
                          "version", VERSION,
                          "copyright", "desenvc team 2012",
                          NULL);

    if (license != NULL)
        free(license);
}

static void add_collection(GtkWidget *w __attribute__((unused)),
    gpointer data __attribute__((unused)))
{
    struct db_collection *c=NULL;

    if (!check_unsaved_data(MSG_BLOCK_APP))
        return;

    c = do_add_dialog(__main_window, NULL);

    if (c != NULL) {
        /*
         * Passes the @__notebook as @user_data to insert the collection in the
         * main's collection list.
         */
        ui_create_notebook(c, __notebook);
    }
}

static char *choose_collection(const char *title)
{
    GtkWidget *dialog, *dlg_box, *frame, *combo;
    GList *list=NULL;
    int result;
    const char *entry;
    char *s=NULL;
    struct db_collection *c;

    dialog = gtk_dialog_new_with_buttons(title,
                                         GTK_WINDOW(__main_window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         NULL);

    gtk_widget_set_size_request(dialog, 500, 100);
    dlg_box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    frame = gtk_frame_new(gettext("Collection's name"));
    combo = gtk_combo_box_text_new();

    for (list = g_list_first(__db_collection); list; list = list->next) {
        c = (struct db_collection *)list->data;
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), c->screen_name);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_container_add(GTK_CONTAINER(frame), combo);

    gtk_box_pack_start(GTK_BOX(dlg_box), frame, FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT) {
        entry = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
        s = strdup(entry);
    }

    g_list_free(list);
    gtk_widget_destroy(dialog);

    return s;
}

static void change_collection(GtkWidget *w __attribute__((unused)),
    gpointer data __attribute__((unused)))
{
    int index=0;
    char *db_screen_name, *db_name;
    struct db_collection *c, *new_c;

    if (!check_unsaved_data(MSG_BLOCK_APP))
        return;

    db_screen_name = choose_collection(gettext("Select collection to update"));

    if (db_screen_name != NULL) {
        db_name = screen_name_to_name(db_screen_name);
        c = search_db_collection_list(db_name, &index);
        new_c = do_add_dialog(__main_window, c);

        if (new_c != NULL) {
            ui_remove_notebook(db_name, __notebook, FALSE);
            ui_create_notebook(new_c, &index);
        }

        free(db_screen_name);
        free(db_name);
    }
}

static void del_collection(GtkWidget *w __attribute__((unused)),
    gpointer data __attribute__((unused)))
{
    char *db_screen_name, *db_name;

    if (!check_unsaved_data(MSG_BLOCK_APP))
        return;

    db_screen_name = choose_collection(gettext("Select collection to delete"));

    if (db_screen_name != NULL) {
        if (!choose_msg(gettext("Attention"),
                        gettext("Are you sure to delete '%s' collection?"),
                        db_screen_name))
        {
            return;
        }

        db_name = screen_name_to_name(db_screen_name);
        ui_remove_notebook(db_name, __notebook, TRUE);
        free(db_name);
        free(db_screen_name);
    }
}

static GtkWidget *ui_create_menu(GtkWidget *window, GtkUIManager *ui_manager)
{
    GtkActionGroup *action_group;
    GtkWidget *menu;
    GError *error=0;

    action_group = gtk_action_group_new("Menu");
    gtk_action_group_add_actions(action_group, __menu_items, __nmenu_items, 0);

    gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
    gtk_ui_manager_add_ui_from_string(GTK_UI_MANAGER(ui_manager), __ui_string, -1,
                                      &error);

    gtk_window_add_accel_group(GTK_WINDOW(window),
                               gtk_ui_manager_get_accel_group(ui_manager));

    menu = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");

    return menu;
}

static gboolean s_delete_event(GtkWidget *w __attribute__((unused)),
    GdkEvent *event __attribute__((unused)),
    gpointer data __attribute__((unused)))
{
    if (!check_unsaved_data(MSG_GIVE_CHOICE))
        return TRUE;

    get_window_settings();

    return FALSE;
}

static gboolean s_window_state_event(GtkWidget *window __attribute__((unused)),
    GdkEventWindowState *event, gpointer data __attribute__((unused)))
{
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
        __settings->maximized = TRUE;
    else
        __settings->maximized = FALSE;

    return TRUE;
}

static void ui_set_window_size(GtkWidget *window, struct app_settings *settings)
{
    gtk_widget_set_size_request(window, settings->wnd_width, settings->wnd_height);

    if (settings->maximized == TRUE)
        gtk_window_maximize(GTK_WINDOW(window));
}

void init_ui(int *argcp, char ***argvp, struct app_settings *settings)
{
    GtkWidget *window, *notebook, *menubar, *vbox;
    GtkUIManager *ui_manager;

    gtk_init(argcp, argvp);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    ui_set_window_size(window, settings);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "delete-event", G_CALLBACK(s_delete_event), NULL);
    g_signal_connect(window, "window-state-event", G_CALLBACK(s_window_state_event),
                     NULL);

    __main_window = window;
    ui_prepend_mainwindow(window);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    ui_manager = gtk_ui_manager_new();
    menubar = ui_create_menu(window, ui_manager);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    __vpane = gtk_vpaned_new();
    gtk_box_pack_start(GTK_BOX(vbox), __vpane, TRUE, TRUE, 0);

    __hpane = gtk_hpaned_new();
    gtk_paned_add1(GTK_PANED(__vpane), __hpane);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_paned_add1(GTK_PANED(__hpane), notebook);

    /* Create a tab for every collection that has been found */
    __db_collection = db_get_all_collection_info();
    __notebook = notebook;
    g_list_foreach(__db_collection, (GFunc)ui_create_notebook, NULL);

    gtk_widget_show_all(window);
    __settings = settings;
}

void run_ui(struct app_settings *settings)
{
    if ((settings->pos_x != -1) && (settings->pos_y != -1))
        gtk_window_move(GTK_WINDOW(__main_window), settings->pos_x, settings->pos_y);

    gtk_main();
}

void exit_ui(void)
{
    ui_remove_mainwindow(__main_window);
    g_list_free(__dlg_data);
    g_list_foreach(__db_collection, (GFunc)destroy_db_collection, NULL);
}

