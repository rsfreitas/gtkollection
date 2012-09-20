
/*
 * Description: control of the main widget of each tab.
 */

#include <stdlib.h>
#include <string.h>
#include <libintl.h>

#include "gtkollection.h"
#include "../misc/cover_image.xpm"

#define TITLE_DEFAULT               0
#define TITLE_UNSAVED               1

#define DATA_SAVED                  0
#define DATA_UNSAVED                1

struct array_sort_model {
    int         column_idx;
    GtkSortType s_type;
};

static int compare_lines(struct dlg_line *line_a, struct dlg_line *line_b,
    struct array_sort_model *s_model)
{
    char *a, *b;

    a = (char *)g_list_nth_data(line_a->column, s_model->column_idx);
    b = (char *)g_list_nth_data(line_b->column, s_model->column_idx);

    return (s_model->s_type == GTK_SORT_ASCENDING) ? strcmp(a, b) : strcmp(b, a);
}

static GString *create_tab_title(struct dlg_data *dlg_data, int data_type)
{
    GString *s;

    s = g_string_new(NULL);
    g_string_printf(s, gettext("%s - %d Items"), dlg_data->c->screen_name,
                    dlg_data->c->n_entries);

    if (data_type == DATA_UNSAVED)
        g_string_append_printf(s, " *");

    return s;
}

static void ui_update_data_status(struct dlg_data *dlg_data, int data_status)
{
    GString *tab_label;

    tab_label = create_tab_title(dlg_data, data_status);
    gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(dlg_data->notebook),
                                    dlg_data->page, tab_label->str);

    g_string_free(tab_label, TRUE);
    gtk_widget_set_sensitive(dlg_data->bt_save,
                             (data_status == DATA_SAVED) ? FALSE : TRUE);
}

static GtkTreeModel *create_model(struct dlg_data *dlg_data)
{
    GType *types;
    GtkListStore *store;
    GList *l;
    int i;
    struct db_field *f;

    types = g_malloc(sizeof(GType) * dlg_data->c->active_fields);

    for (l = g_list_first(dlg_data->c->fields), i = 0; l; l = l->next) {
        f = (struct db_field *)l->data;

        if (f->status == FIELD_ACTIVE) {
            types[i] = G_TYPE_STRING;
            i++;
        }
    }

    store = gtk_list_store_newv(dlg_data->c->active_fields, types);
    db_load_and_set_collection_data(dlg_data->c, store, dlg_data);
    g_free(types);

    return GTK_TREE_MODEL(store);
}

static void tree_add_column(struct db_field *f, struct dlg_data *dlg_data)
{
    GtkCellRenderer *renderer;

    if (f->status == FIELD_HIDDEN)
        return;

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(dlg_data->priv.treeview),
                                                -1, f->screen_name, renderer,
                                                "text", dlg_data->priv.tab_column_idx,
                                                NULL);

    dlg_data->priv.tab_column_idx++;
}

static void dlg_insert_text_entries(struct dlg_data *dlg_data,
    GtkWidget **textbox, GtkTreePath *path)
{
    GtkTreeIter iter;
    int i;
    char *s;

    gtk_tree_model_get_iter(dlg_data->priv.model, &iter, path);

    for (i = 0; i < dlg_data->c->active_fields; i++) {
        gtk_tree_model_get(dlg_data->priv.model, &iter, i, &s, -1);
        gtk_entry_set_text(GTK_ENTRY(textbox[i]), s);
    }
}

static int dlg_update_entry(struct dlg_data *dlg_data, GtkWidget **textbox,
    struct dlg_line *line, GtkTreePath *path)
{
    int i;
    GList *l;
    const char *s;
    GtkTreeIter iter;

    gtk_tree_model_get_iter(dlg_data->priv.model, &iter, path);

    /* For new entries only updates its columns and don't updates @line->status */
    if (line->status == LINE_ADDED) {
        for (l = g_list_first(line->column), i = 0; l; l = l->next, i++) {
            free(l->data);
            s = gtk_entry_get_text(GTK_ENTRY(textbox[i]));
            l->data = strdup(s);
        }
    } else {
        /* Entries loaded from database have their status updated */
        line->status = LINE_UPDATED;

        if (line->new_column != NULL) {
            g_list_free(line->new_column);
            line->new_column = NULL;
        }

        for (i = 0; i < dlg_data->c->active_fields; i++) {
            s = gtk_entry_get_text(GTK_ENTRY(textbox[i]));
            line->new_column = g_list_append(line->new_column, strdup(s));
        }
    }

    if (dlg_data->priv.bt_img_filename != NULL) {
        if (line->img_filename != NULL)
            free(line->img_filename);

        line->img_filename = strdup(dlg_data->priv.bt_img_filename);
    }

    for (i = 0; i < dlg_data->c->active_fields; i++) {
        s = gtk_entry_get_text(GTK_ENTRY(textbox[i]));
        gtk_list_store_set(GTK_LIST_STORE(dlg_data->priv.model), &iter, i, s, -1);
    }

    return 1;
}

static int dlg_add_entry(struct dlg_data *dlg_data, GtkWidget **textbox)
{
    struct dlg_line *line;
    const char *s;
    int i;
    GtkTreeIter iter;

    gtk_list_store_append(GTK_LIST_STORE(dlg_data->priv.model), &iter);
    line = create_dlg_line(LINE_ADDED);

    if (!line) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error creating structure for new line!"));

        return 0;
    }

    for (i = 0; i < dlg_data->c->active_fields; i++) {
        s = gtk_entry_get_text(GTK_ENTRY(textbox[i]));
        gtk_list_store_set(GTK_LIST_STORE(dlg_data->priv.model), &iter, i, s, -1);
        line->column = g_list_append(line->column, strdup(s));
    }

    if (dlg_data->priv.bt_img_filename != NULL)
        line->img_filename = strdup(dlg_data->priv.bt_img_filename);
    else
        line->img_filename = strdup("default_image_xpm");

    g_array_append_vals(dlg_data->priv.data, line, 1);

    return 1;
}

static int dlg_check_filled_fields(GtkWidget **textbox, int n_textbox)
{
    int i;
    const char *s;

    for (i = 0; i < n_textbox; i++) {
        s = gtk_entry_get_text(GTK_ENTRY(textbox[i]));

        if (!strlen(s))
            return 0;
    }

    return 1;
}

static void s_bt_img_clicked(GtkButton *button __attribute__((unused)),
    struct dlg_data *dlg_data)
{
    GtkWidget *image;
    GdkPixbuf *pixbuf;
    char *filename;
    struct dlg_line *line;
    const char *s;
    int i;

    line = create_dlg_line(LINE_ADDED);

    if (!line) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error creating structure for new line!"));

        return;
    }

    for (i = 0; i < dlg_data->c->active_fields; i++) {
        s = gtk_entry_get_text(GTK_ENTRY(dlg_data->priv.textbox[i]));
        line->column = g_list_append(line->column, strdup(s));
    }

    filename = get_cover_image_file(dlg_data->c, line);

    if (filename != NULL) {
        image = gtk_image_new_from_file(filename);
        pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
        pixbuf = gdk_pixbuf_scale_simple(pixbuf, 150, 150, GDK_INTERP_BILINEAR);
        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);

        gtk_button_set_label(GTK_BUTTON(dlg_data->priv.bt_img), "");
        gtk_button_set_image(GTK_BUTTON(dlg_data->priv.bt_img), image);

        dlg_data->priv.bt_img_filename = strdup(filename);
        g_free(filename);
    }
}

static void do_entry_dialog(struct dlg_data *dlg_data, int dialog_type,
    struct dlg_line *line, GtkTreePath *path)
{
    GtkWidget *dialog, *dlg_box, **textbox, **frame, *bt_img, *hbox, *vbox, *image;
    GdkPixbuf *pixbuf;
    int result, i, j, ret, loop=1;
    struct db_field *f;
    GError *error=NULL;

    dialog = gtk_dialog_new_with_buttons((dialog_type == DLG_ADD_ENTRY)
                                             ? gettext("Add new entry")
                                             : gettext("Update entry"),
                                         GTK_WINDOW(ui_get_mainwindow()),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         NULL);

    gtk_widget_set_size_request(dialog, 400, 300);
    dlg_box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    hbox = gtk_hbox_new(FALSE, 3);
    vbox = gtk_vbox_new(FALSE, 3);

    gtk_box_pack_start(GTK_BOX(dlg_box), hbox, TRUE, TRUE, 0);

    /* create cover image button */
    if (line != NULL) {
        image = gtk_image_new();

        if (!strcmp(line->img_filename, "default_image_xpm"))
            pixbuf = gdk_pixbuf_new_from_xpm_data(__default_cover_image);
        else {
            pixbuf = gdk_pixbuf_new_from_file(line->img_filename, &error);

            if (NULL == pixbuf) {
                g_error_free(error);
                pixbuf = gdk_pixbuf_new_from_xpm_data(__default_cover_image);
            }

            pixbuf = gdk_pixbuf_scale_simple(pixbuf, 150, 150, GDK_INTERP_BILINEAR);
        }

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
        bt_img = gtk_button_new();
        gtk_button_set_image(GTK_BUTTON(bt_img), image);
    } else
        bt_img = gtk_button_new_with_label("Cover");

    g_signal_connect(bt_img, "clicked", G_CALLBACK(s_bt_img_clicked), dlg_data);
    gtk_box_pack_start(GTK_BOX(hbox), bt_img, TRUE, TRUE, 0);
    dlg_data->priv.bt_img = bt_img;

    /* create textboxes */
    textbox = malloc(sizeof(GtkWidget *) * dlg_data->c->active_fields);
    frame = malloc(sizeof(GtkWidget *) * dlg_data->c->active_fields);

    for (i = 0, j = 0; i < dlg_data->c->n_fields; i++) {
        f = g_list_nth_data(dlg_data->c->fields, i);

        if (f->status == FIELD_HIDDEN)
            continue;

        frame[j] = gtk_frame_new(f->screen_name);
        textbox[j] = gtk_entry_new();

        gtk_container_add(GTK_CONTAINER(frame[j]), textbox[j]);
        gtk_container_add(GTK_CONTAINER(vbox), frame[j]);

        j++;
    }

    dlg_data->priv.textbox = textbox;

    if (dialog_type == DLG_UPDATE_ENTRY)
        dlg_insert_text_entries(dlg_data, textbox, path);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    do {
        result = gtk_dialog_run(GTK_DIALOG(dialog));

        if (result == GTK_RESPONSE_ACCEPT) {
            if ((dialog_type == DLG_ADD_ENTRY) &&
                !dlg_check_filled_fields(textbox, dlg_data->c->active_fields))
            {
                display_msg(GTK_MESSAGE_WARNING, gettext("Warning"),
                            gettext("All fields must be filled to add a new entry!"));

                continue;
            }

            if (dialog_type == DLG_ADD_ENTRY)
                ret = dlg_add_entry(dlg_data, textbox);
            else
                ret = dlg_update_entry(dlg_data, textbox, line, path);

            if (ret) {
                ui_update_data_status(dlg_data, DATA_UNSAVED);
                loop = 0;
            }
        } else
            loop = 0;
    } while (loop);

    gtk_widget_destroy(dialog);
    free(frame);
    free(textbox);
}

static void s_tree_edit_row(GtkTreeView *treeview __attribute__((unused)),
    GtkTreePath *path, GtkTreeViewColumn *column __attribute__((unused)),
    struct dlg_data *dlg_data)
{
    int model_idx;
    struct dlg_line *line;

    model_idx = gtk_tree_path_get_indices(path)[0];
    line = &g_array_index(dlg_data->priv.data, dlg_line, model_idx);

    if (dlg_data->priv.bt_img_filename != NULL) {
        free(dlg_data->priv.bt_img_filename);
        dlg_data->priv.bt_img_filename = NULL;
    }

    do_entry_dialog(dlg_data, DLG_UPDATE_ENTRY, line, path);
}

static void s_bt_add_clicked(GtkButton *button __attribute__((unused)),
    struct dlg_data *dlg_data)
{
    if (dlg_data->priv.bt_img_filename != NULL) {
        free(dlg_data->priv.bt_img_filename);
        dlg_data->priv.bt_img_filename = NULL;
    }

    do_entry_dialog(dlg_data, DLG_ADD_ENTRY, NULL, NULL);
}

static void s_bt_del_clicked(GtkButton *button __attribute__((unused)),
    struct dlg_data *dlg_data)
{
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreePath *path;
    int model_idx;
    unsigned long long id;
    char *img_filename;
    struct dlg_line *line;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg_data->priv.treeview));

    if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        if (!choose_msg(gettext("Attention"),
                        gettext("Are you sure to remove the selected entry?")))
        {
            return;
        }

        path = gtk_tree_model_get_path(dlg_data->priv.model, &iter);
        model_idx = gtk_tree_path_get_indices(path)[0];
        line = &g_array_index(dlg_data->priv.data, dlg_line, model_idx);
        img_filename = strdup(line->img_filename);
        id = line->id;

        /* Gets only entries that were loaded from database */
        if (line->status != LINE_ADDED) {
            line = create_dlg_line(LINE_DELETED);

            if (!line) {
                display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                            gettext("Error creating structure for removed line!"));

                return;
            }

            line->img_filename = strdup(img_filename);
            line->id = id;

            dlg_data->priv.d_lines = g_list_append(dlg_data->priv.d_lines, line);
        }

        free(img_filename);
        gtk_list_store_remove(GTK_LIST_STORE(dlg_data->priv.model), &iter);
        gtk_tree_path_free(path);
        g_array_remove_index(dlg_data->priv.data, model_idx);

        ui_update_data_status(dlg_data, DATA_UNSAVED);
    }
}

static void s_bt_save_clicked(GtkButton *button __attribute__((unused)),
    struct dlg_data *dlg_data)
{
    int d_lines=0, a_lines=0;

    /* remove lines */
    if (dlg_data->priv.d_lines != NULL) {
        d_lines = db_delete_collection_data(dlg_data->c, dlg_data->priv.d_lines);

        /* clear the list so we don't try to remove them again */
        g_list_free_full(dlg_data->priv.d_lines, (GDestroyNotify)destroy_dlg_line);
        dlg_data->priv.d_lines = NULL;
    }

    /* update all remaining lines */
    a_lines = db_update_collection_data(dlg_data->c, dlg_data->priv.data);

    if (a_lines || d_lines)
        dlg_data->c->n_entries += (a_lines - d_lines);

    ui_update_data_status(dlg_data, DATA_SAVED);
}

static void s_tree_line_selected(GtkTreeSelection *selection,
    struct dlg_data *dlg_data)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    GdkPixbuf *pixbuf;
    int model_idx;
    struct dlg_line *line;
    GError *error=0;

    if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        path = gtk_tree_model_get_path(dlg_data->priv.model, &iter);
        model_idx = gtk_tree_path_get_indices(path)[0];
        line = &g_array_index(dlg_data->priv.data, dlg_line, model_idx);

        if (!strcmp(line->img_filename, "default_image_xpm"))
            pixbuf = gdk_pixbuf_new_from_xpm_data(__default_cover_image);
        else {
            pixbuf = gdk_pixbuf_new_from_file(line->img_filename, &error);

            if (NULL == pixbuf) {
                g_error_free(error);
                pixbuf = gdk_pixbuf_new_from_xpm_data(__default_cover_image);
            }

            pixbuf = gdk_pixbuf_scale_simple(pixbuf, 150, 150, GDK_INTERP_BILINEAR);
        }

        gtk_image_set_from_pixbuf(GTK_IMAGE(dlg_data->priv.image), pixbuf);
        gtk_widget_show_all(dlg_data->priv.image);
    }
}

static void update_treeview_data(struct dlg_data *dlg_data)
{
    struct array_sort_model s_model;
    unsigned int i, j;
    GList *l;
    GtkTreeIter iter;
    struct dlg_line *line;

    s_model.column_idx =
        gtk_combo_box_get_active(GTK_COMBO_BOX(dlg_data->priv.sort_combo));

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlg_data->priv.rd_asc)))
        s_model.s_type = GTK_SORT_ASCENDING;
    else
        s_model.s_type = GTK_SORT_DESCENDING;

    g_array_sort_with_data(dlg_data->priv.data, (GCompareDataFunc)compare_lines,
                           &s_model);

    gtk_list_store_clear(GTK_LIST_STORE(dlg_data->priv.model));

    for (i = 0; i < dlg_data->priv.data->len; i++) {
        line = &g_array_index(dlg_data->priv.data, dlg_line, i);
        gtk_list_store_append(GTK_LIST_STORE(dlg_data->priv.model), &iter);

        for (l = g_list_first(line->column), j = 0; l; l = l->next, j++) {
            gtk_list_store_set(GTK_LIST_STORE(dlg_data->priv.model), &iter, j,
                               (char *)l->data, -1);
        }
    }
}

static void s_enable_sorting(GtkWidget *w, struct dlg_data *dlg_data)
{
    if (GTK_TOGGLE_BUTTON(w)->active) {
        update_treeview_data(dlg_data);

        gtk_widget_set_sensitive(dlg_data->priv.sort_combo, TRUE);
        gtk_widget_set_sensitive(dlg_data->priv.rd_asc, TRUE);
        gtk_widget_set_sensitive(dlg_data->priv.rd_desc, TRUE);
    } else {
        gtk_widget_set_sensitive(dlg_data->priv.sort_combo, FALSE);
        gtk_widget_set_sensitive(dlg_data->priv.rd_asc, FALSE);
        gtk_widget_set_sensitive(dlg_data->priv.rd_desc, FALSE);
    }
}

static void s_sort_combo_changed(GtkWidget *w __attribute__((unused)),
    struct dlg_data *dlg_data)
{
    if (dlg_data->priv.data->len > 0)
        update_treeview_data(dlg_data);
}

static GtkWidget *dlg_create_sorting_widgets(struct dlg_data *dlg_data)
{
    GtkWidget *frame, *vbox, *check_bt, *combo, *rd_asc, *rd_desc;
    struct db_field *f;
    GList *l;

    frame = gtk_frame_new(gettext("Sorting options"));
    vbox = gtk_vbox_new(FALSE, 3);
    check_bt = gtk_check_button_new_with_label(gettext("Enable sorting"));
    g_signal_connect(check_bt, "toggled", G_CALLBACK(s_enable_sorting),
                     dlg_data);

    combo = gtk_combo_box_text_new();
    g_signal_connect(combo, "changed", G_CALLBACK(s_sort_combo_changed),
                     dlg_data);

    for (l = g_list_first(dlg_data->c->fields); l; l = l->next) {
        f = (struct db_field *)l->data;

        if (f->status == FIELD_ACTIVE)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo),
                                           f->screen_name);
    }

    gtk_widget_set_sensitive(combo, FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), dlg_data->info.idx_field);
    rd_asc = gtk_radio_button_new_with_label(NULL, gettext("Ascending"));
    rd_desc = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rd_asc),
                                                          gettext("Descending"));

    /* Call this before installing the signal */
    if (dlg_data->info.order == CONFIG_SORT_ASC)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rd_asc), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rd_desc), TRUE);

    g_signal_connect(rd_asc, "toggled", G_CALLBACK(s_enable_sorting), dlg_data);
    g_signal_connect(rd_desc, "toggled", G_CALLBACK(s_enable_sorting), dlg_data);

    gtk_widget_set_sensitive(rd_asc, FALSE);
    gtk_widget_set_sensitive(rd_desc, FALSE);

    gtk_box_pack_start(GTK_BOX(vbox), check_bt, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), rd_asc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), rd_desc, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(frame), vbox);

    dlg_data->priv.sort_combo = combo;
    dlg_data->priv.rd_asc = rd_asc;
    dlg_data->priv.rd_desc = rd_desc;
    dlg_data->priv.check_bt_sort = check_bt;

    return frame;
}

static GtkWidget *dlg_create_buttons_widgets(struct dlg_data *dlg_data)
{
    GtkWidget *vbox_bt, *bt_add, *bt_add_img, *bt_del, *bt_del_img, *bt_save,
        *bt_save_img;

    vbox_bt = gtk_vbox_new(FALSE, 3);

    bt_add = gtk_button_new_with_label(gettext("Add entry"));
    bt_add_img = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(bt_add), bt_add_img);
    g_signal_connect(bt_add, "clicked", G_CALLBACK(s_bt_add_clicked), dlg_data);
    gtk_box_pack_start(GTK_BOX(vbox_bt), bt_add, TRUE, TRUE, 0);

    bt_del = gtk_button_new_with_label(gettext("Remove entry"));
    bt_del_img = gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(bt_del), bt_del_img);
    g_signal_connect(bt_del, "clicked", G_CALLBACK(s_bt_del_clicked), dlg_data);
    gtk_box_pack_start(GTK_BOX(vbox_bt), bt_del, TRUE, TRUE, 0);

    bt_save = gtk_button_new_with_label(gettext("Save modifications"));
    bt_save_img = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(bt_save), bt_save_img);
    g_signal_connect(bt_save, "clicked", G_CALLBACK(s_bt_save_clicked), dlg_data);
    gtk_box_pack_start(GTK_BOX(vbox_bt), bt_save, TRUE, TRUE, 0);

    dlg_data->bt_save = bt_save;

    return vbox_bt;
}

static GtkWidget *dlg_create_treeview(struct dlg_data *dlg_data)
{
    GtkWidget *sw, *treeview;
    GtkTreeModel *model;
    GtkTreeSelection *selection;

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                        GTK_SHADOW_ETCHED_IN);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    treeview = gtk_tree_view_new();
    model = create_model(dlg_data);

    dlg_data->priv.treeview = treeview;
    dlg_data->priv.model = model;
    g_signal_connect(treeview, "row-activated", G_CALLBACK(s_tree_edit_row),
                     dlg_data);

    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model);
    dlg_data->priv.tab_column_idx = 0;
    g_list_foreach(dlg_data->c->fields, (GFunc)tree_add_column, dlg_data);
    g_object_unref(model);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(selection, "changed", G_CALLBACK(s_tree_line_selected),
                     dlg_data);

    gtk_container_add(GTK_CONTAINER(sw), treeview);

    return sw;
}

struct dlg_data *collection_widget(struct db_collection *c, GtkWidget *notebook)
{
    GtkWidget *vbox, *sw, *vbox_bt, *image, *hbox, *vbox_cinfo, *cinfo;
    struct dlg_data *dlg_data;

    dlg_data = create_dlg_data();

    if (!dlg_data)
        return NULL;

    dlg_data->notebook = notebook;
    dlg_data->c = c;
    load_collection_info_from_config(c->name, &dlg_data->info);

    vbox = gtk_vbox_new(FALSE, 3);
    hbox = gtk_hbox_new(FALSE, 3);
    vbox_cinfo = gtk_vbox_new(FALSE, 3);

    /* create widgets for sorting */
    cinfo = dlg_create_sorting_widgets(dlg_data);
    gtk_box_pack_start(GTK_BOX(vbox_cinfo), cinfo, FALSE, FALSE, 0);

    /* create cover image */
    image = gtk_image_new_from_pixbuf(NULL);
    dlg_data->priv.image = image;
    gtk_box_pack_start(GTK_BOX(vbox_cinfo), image, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), vbox_cinfo, FALSE, FALSE, 0);

    /* create treeview */
    sw = dlg_create_treeview(dlg_data);
    gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    /* create_buttons */
    vbox_bt = dlg_create_buttons_widgets(dlg_data);
    gtk_box_pack_start(GTK_BOX(vbox), vbox_bt, FALSE, FALSE, 0);

    /*
     * Call this here because the collection information has already been loaded
     * from database and we can sort them.
     */
    if (dlg_data->info.enable)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg_data->priv.check_bt_sort),
                                     TRUE);

    dlg_data->page = vbox;
    ui_update_data_status(dlg_data, DATA_SAVED);

    return dlg_data;
}

