
/*
 * Description: dialog for manipulating collections.
 */

#include <stdlib.h>
#include <string.h>
#include <libintl.h>

#include "gtkollection.h"

#define MAX_TREE_COLUMNS                2

enum columns {
    COLUMN_NAME = 0,
    COLUMN_FIELD_STATUS
};

struct collection_dlg_st {
    GtkWidget   *treeview;
    int         dlg_type;
    int         rows;
};

static int status_str_to_int(const char *s)
{
    if (!strcmp(s, "active"))
        return FIELD_ACTIVE;
    else if (!strcmp(s, "hidden"))
        return FIELD_HIDDEN;

    return FIELD_ACTIVE;
}

static char *status_int_to_str(int field_status)
{
    char s[32]={0};

    switch (field_status) {
        case FIELD_ACTIVE:
            strcpy(s, "active");
            break;

        case FIELD_HIDDEN:
            strcpy(s, "hidden");
            break;
    }

    return strdup(s);
}

static GtkTreeModel *dlg_create_model(void)
{
    GtkListStore *store;

    store = gtk_list_store_new(MAX_TREE_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

    return GTK_TREE_MODEL(store);
}

static void s_cell_edited(GtkCellRenderer *cell, const gchar *path_string,
    const gchar *new_text, GtkTreeModel *model)
{
    GtkTreeIter iter;
    int column;

    if (!gtk_tree_model_get_iter_from_string(model, &iter, path_string))
        return;

    column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));

    switch (column) {
        case COLUMN_NAME:
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, new_text, -1);
            break;

        case COLUMN_FIELD_STATUS:
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, new_text, -1);
            break;
    }
}

static void add_columns(GtkTreeView *treeview, GtkTreeModel *model)
{
    GtkCellRenderer *renderer;
    GtkListStore *store_combo;
    GtkTreeIter iter;

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", G_CALLBACK(s_cell_edited), model);
    g_object_set_data(G_OBJECT(renderer), "column", GINT_TO_POINTER(COLUMN_NAME));
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "Name",
                                                renderer, "text", COLUMN_NAME,
                                                NULL);

    renderer = gtk_cell_renderer_combo_new();
    store_combo = gtk_list_store_new(1, G_TYPE_STRING);

    gtk_list_store_append(store_combo, &iter);
    gtk_list_store_set(store_combo, &iter, 0, "active", -1);
    gtk_list_store_append(store_combo, &iter);
    gtk_list_store_set(store_combo, &iter, 0, "hidden", -1);

    g_object_set(renderer, "model", store_combo, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", G_CALLBACK(s_cell_edited), model);
    g_object_set_data(G_OBJECT(renderer), "column",
                      GINT_TO_POINTER(COLUMN_FIELD_STATUS));

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1,
                                                "Field status", renderer,
                                                "text", COLUMN_FIELD_STATUS,
                                                "text-column", 0,
                                                NULL);
}

static void s_bt_add_clicked(GtkButton *button __attribute__((unused)),
    GtkTreeModel *model)
{
    GtkTreeIter iter;

    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_NAME,
                       "Field Name", -1);

    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_FIELD_STATUS,
                       "active", -1);
}

static void s_bt_del_clicked(GtkButton *button __attribute__((unused)),
    struct collection_dlg_st *dlg_st)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    int model_idx;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg_st->treeview));

    if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg_st->treeview));
        path = gtk_tree_model_get_path(model, &iter);
        model_idx = gtk_tree_path_get_indices(path)[0];

        /* Only removes fields that had not been added */
        if (model_idx <= (dlg_st->rows - 1)) {
            display_msg(GTK_MESSAGE_WARNING, gettext("Warning"),
                        gettext("Only newer (unsaved) fields may be removed."));

            gtk_tree_path_free(path);
            return;
        }

        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        gtk_tree_path_free(path);
    }
}

static void add_treeview_data(GtkTreeView *treeview, struct db_collection *db)
{
    GList *fields;
    GtkTreeModel *model;
    GtkTreeIter iter;
    struct db_field *f;
    char *status;

    model = gtk_tree_view_get_model(treeview);

    for (fields = g_list_first(db->fields); fields; fields = fields->next) {
        f = (struct db_field *)fields->data;
        status = status_int_to_str(f->status);

        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_NAME, f->screen_name,
                           COLUMN_FIELD_STATUS, status, -1);

        free(status);
    }
}

static struct db_collection *get_db_collection_from_dialog(GtkTreeModel *model,
    GtkWidget *text_entry)
{
    gboolean valid;
    GtkTreeIter iter;
    struct db_collection *c=NULL;
    struct db_field *f;
    int i=0;
    char *name, *screen_name, *status;
    const char *db_screen_name=NULL;

    db_screen_name = gtk_entry_get_text(GTK_ENTRY(text_entry));

    if (!strlen(db_screen_name)) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Invalid collection's name!"));

        return NULL;
    }

    valid = gtk_tree_model_get_iter_first(model, &iter);

    /* no field has been added */
    if (!valid) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("No field has been added on the new collection!"));

        return NULL;
    }

    c = create_db_collection(db_screen_name, NULL, -1);

    while (valid) {
        gtk_tree_model_get(model, &iter,
                           COLUMN_NAME, &screen_name,
                           COLUMN_FIELD_STATUS, &status, -1);

        name = screen_name_to_name(screen_name);
        f = create_db_field(name, screen_name, i, status_str_to_int(status));
        add_db_field(c, f);
        i++;

        free(status);
        free(screen_name);
        free(name);

        valid = gtk_tree_model_iter_next(model, &iter);
    }

    end_add_db_field(c);
    return c;
}

static int list_names_compare(const char *a, const char *b)
{
    return strcmp(a, b);
}

static int dlg_has_repeated_entries(GtkTreeModel *model)
{
    gboolean valid;
    char *screen_name;
    GList *l_names=NULL;
    GtkTreeIter iter;
    int ret=0;

    valid = gtk_tree_model_get_iter_first(model, &iter);

    /* no field has been added */
    if (!valid) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("No field has been added on the new collection!"));

        return 1;
    }

    while (valid) {
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &screen_name, -1);

        if (l_names != NULL) {
            if (g_list_find_custom(l_names, screen_name,
                                   (GCompareFunc)list_names_compare) != NULL)
            {
                display_msg(GTK_MESSAGE_WARNING, gettext("Attention"),
                            gettext("The collection cannot have duplicate fields!"));

                free(screen_name);
                ret = 1;
                break;
            }
        }

        l_names = g_list_append(l_names, screen_name);
        valid = gtk_tree_model_iter_next(model, &iter);
    }

    g_list_free(l_names);
    return ret;
}

struct db_collection *do_add_dialog(GtkWidget *main_window, struct db_collection *db)
{
    GtkTreeModel *model;
    GtkWidget *dialog, *dlg_box, *sw, *treeview, *vbox_bt, *bt_add, *bt_del, *hbox,
        *vbox, *frame, *t_entry;
    int result, loop=1;
    struct db_collection *c=NULL;
    struct collection_dlg_st *dlg_st;

    dlg_st = g_malloc(sizeof(struct collection_dlg_st));
    dialog = gtk_dialog_new_with_buttons((db == NULL) ? gettext("Add new collection")
                                                      : gettext("Update collection's structure"),
                                         GTK_WINDOW(main_window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         NULL);

    gtk_widget_set_size_request(dialog, 500, 300);
    dlg_box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    if (db == NULL) {
        dlg_st->dlg_type = DLG_ADD_ENTRY;
        dlg_st->rows = 0;
    } else {
        dlg_st->dlg_type = DLG_UPDATE_ENTRY;
        dlg_st->rows = db->n_fields;
    }

    /* create container to hold the treeview and buttons */
    hbox = gtk_hbox_new(FALSE, 3);

    /* create treeview */
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                        GTK_SHADOW_ETCHED_IN);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);
    treeview = gtk_tree_view_new();
    dlg_st->treeview = treeview;
    model = dlg_create_model();

    add_columns(GTK_TREE_VIEW(treeview), model);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model);

    if (db != NULL)
        add_treeview_data(GTK_TREE_VIEW(treeview), db);

    gtk_container_add(GTK_CONTAINER(sw), treeview);

    /* create_buttons */
    vbox_bt = gtk_vbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(hbox), vbox_bt);

    bt_add = gtk_button_new_with_label(gettext("Add field"));
    g_signal_connect(bt_add, "clicked", G_CALLBACK(s_bt_add_clicked),
                     gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));

    gtk_box_pack_start(GTK_BOX(vbox_bt), bt_add, FALSE, FALSE, 0);

    bt_del = gtk_button_new_with_label(gettext("Remove field"));
    g_signal_connect(bt_del, "clicked", G_CALLBACK(s_bt_del_clicked), dlg_st);
    gtk_box_pack_start(GTK_BOX(vbox_bt), bt_del, FALSE, FALSE, 0);

    /* create container to textbox + hbox */
    vbox = gtk_vbox_new(FALSE, 3);

    /* create textbox */
    frame = gtk_frame_new(gettext("Collection's name"));
    t_entry = gtk_entry_new();

    if (db != NULL)
        gtk_entry_set_text(GTK_ENTRY(t_entry), db->screen_name);

    gtk_container_add(GTK_CONTAINER(frame), t_entry);

    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    /* run dialog */
    gtk_container_add(GTK_CONTAINER(dlg_box), vbox);
    gtk_widget_show_all(dialog);
    ui_prepend_mainwindow(dialog);

    do {
        result = gtk_dialog_run(GTK_DIALOG(dialog));

        if (result == GTK_RESPONSE_ACCEPT) {
            model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

            if (dlg_has_repeated_entries(model))
                continue;

            c = get_db_collection_from_dialog(model, t_entry);

            if (c != NULL) {
                if (db == NULL)
                    db_create_collection(c, TRUE);
                else {
                    if (!db_update_collection(c, db)) {
                        destroy_db_collection(c, NULL);
                        c = NULL;
                    }
                }

                loop = 0;
            }
        } else
            loop = 0;
    } while (loop);

    g_free(dlg_st);
    ui_remove_mainwindow(dialog);
    gtk_widget_destroy(dialog);

    return c;
}

