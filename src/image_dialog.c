
/*
 * Description: image search dialog.
 */

#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "gtkollection.h"
#include "../misc/cover_image.xpm"

struct plugin_st {
    int                 n_images;
    unsigned long long  total;
};

static char *create_new_filename(void)
{
    char path[256]={0}, *name;

    name = strrand(13);
    snprintf(path, sizeof(path), "%s/%s", UNSAVED_IMG_TMP_DIR, name);
    free(name);

    return strdup(path);
}

static int run_web_image_plugin(struct plugin_st *p_st, const char *query, int start)
{
    char ret[64]={0}, tmp[32]={0};
    int fd[2], c_status;
    pid_t pid;
    const char *args[] = {
        "/usr/share/gtkollection/pl_images", "/usr/share/gtkollection/pl_images",
        "-q", "-s", NULL
    };

    if (pipe(fd) == -1) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error starting environment to run the image plugin."));

        return 0;
    }

    snprintf(tmp, sizeof(tmp), "%d", start);
    pid = fork();

    if (pid < 0) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error resizing image to temporary file."));

        return 0;
    } else if (pid == 0) {
        dup2(fd[1], 1);
        execl(IMAGE_PLUGIN, IMAGE_PLUGIN, args[2], query, args[3], tmp,
              (char *)NULL);

        close(fd[0]);
        _exit(0);
    }

    close(fd[1]);
    read(fd[0], ret, sizeof(ret) - 1);
    close(fd[0]);
    waitpid(pid, &c_status, WUNTRACED);
    sscanf(ret, "%d,%llu", &p_st->n_images, &p_st->total);

    if (p_st->n_images < 0) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("No network connection. Check your network settings!"));

        return 0;
    }

    return 1;
}

static void s_bt_img_clicked(GtkButton *bt, GtkDialog *dialog)
{
    int id;
    char filename[128]={0};

    id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(bt), "bt-id"));
    snprintf(filename, sizeof(filename), "%s/image_%d.jpg", WEB_IMG_TMP_DIR, id);

    if (!access(filename, 0x00))
        g_signal_emit_by_name(dialog, "response", id);
}

static void dlg_set_bt_img(GtkWidget **bt_img)
{
    GtkWidget *image;
    GdkPixbuf *pixbuf;
    int i;
    char filename[128];

    for (i = 0; (i < 4); i++) {
        memset(filename, 0, sizeof(filename));
        snprintf(filename, sizeof(filename), "%s/image_%d.jpg", WEB_IMG_TMP_DIR, i);

        if (!access(filename, 0x00)) {
            image = gtk_image_new_from_file(filename);
            pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
        } else {
            image = gtk_image_new();
            pixbuf = gdk_pixbuf_new_from_xpm_data(__default_cover_image);
        }

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
        gtk_button_set_label(GTK_BUTTON(bt_img[i]), "");
        gtk_button_set_image(GTK_BUTTON(bt_img[i]), image);
    }
}

static char *web_cover_dlg(const char *query)
{
    GtkWidget *dialog, *dlg_box, *hbox, **bt_img;
    char *filename=NULL, bt_image_name[128]={0};
    int i, loop=1, result, start=0;
    struct plugin_st p_st;

    dialog = gtk_dialog_new_with_buttons(gettext("Select an image"),
                                         GTK_WINDOW(ui_get_mainwindow()),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         gettext("Prev"), GTK_RESPONSE_REJECT,
                                         gettext("Next"), GTK_RESPONSE_NONE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_ACCEPT,
                                         NULL);

    gtk_widget_set_size_request(dialog, 660, 230);
    dlg_box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    hbox = gtk_hbox_new(FALSE, 5);

    /* create buttons */
    bt_img = malloc(sizeof(GtkWidget *) * 4);

    for (i = 0; i < 4; i++) {
        bt_img[i] = gtk_button_new_with_label("Image");
        g_object_set_data(G_OBJECT(bt_img[i]), "bt-id", GINT_TO_POINTER(i));
        g_signal_connect(bt_img[i], "clicked", G_CALLBACK(s_bt_img_clicked), dialog);

        gtk_box_pack_start(GTK_BOX(hbox), bt_img[i], TRUE, TRUE, 0);
    }

    /* run dialog */
    gtk_container_add(GTK_CONTAINER(dlg_box), hbox);
    gtk_widget_show_all(dialog);
    ui_prepend_mainwindow(dialog);

    do {
        if (!run_web_image_plugin(&p_st, query, start))
            break;
        else
            dlg_set_bt_img(bt_img);

        result = gtk_dialog_run(GTK_DIALOG(dialog));

        switch (result) {
            case GTK_RESPONSE_ACCEPT:
                loop = 0;
                break;

            case GTK_RESPONSE_REJECT: /* prev */
                if ((start - 4) < 0)
                    start = 0;
                else
                    start -= 4;

                break;

            case GTK_RESPONSE_NONE: /* next */
                if (p_st.total > (unsigned long long)(start + 4))
                    start += 4;
                else
                    start = p_st.total;

                break;

            case GTK_RESPONSE_DELETE_EVENT:
                loop = 0;
                break;

            default:
                snprintf(bt_image_name, sizeof(bt_image_name),
                         "%s/image_%d.jpg", WEB_IMG_TMP_DIR, result);

                filename = strdup(bt_image_name);
                loop = 0;
                break;
        }
    } while (loop);

    ui_remove_mainwindow(dialog);
    gtk_widget_destroy(dialog);
    free(bt_img);

    return filename;
}

static GString *create_label_text(struct db_collection *c)
{
    GString *s=NULL;
    struct db_field *f;
    GList *l;
    int i;

    s = g_string_new(NULL);
    g_string_printf(s, gettext("These are the options that you may pass to "
                               "search for the image: \n\n"));

    g_string_append_printf(s, "$name - Collection name\n");

    for (l = g_list_first(c->fields), i = 1; l; l = l->next) {
        f = (struct db_field *)l->data;

        if (f->status == FIELD_ACTIVE) {
            g_string_append_printf(s, "$%d - %s\n", i, f->screen_name);
            i++;
        }
    }

    return s;
}

static char *parse_text_entry(const char *text, struct db_collection *c,
    struct dlg_line *line)
{
    GString *s;
    char *r=NULL, token[4];
    int i;

    s = g_string_new(NULL);
    g_string_printf(s, "\"%s", text);
    g_string_replace(s, "$name", c->screen_name);

    for (i = 0; i < c->active_fields; i++) {
        memset(token, 0, sizeof(token));
        snprintf(token, sizeof(token), "$%d", i + 1);

        if (strstr(s->str, token) != NULL) {
            g_string_replace(s, token,
                             (const char *)g_list_nth_data(line->column, i));
        }
    }

    g_string_append_printf(s, "\"");
    r = strdup(s->str);
    g_string_free(s, TRUE);

    return r;
}

static char *web_query_dlg(struct db_collection *c, struct dlg_line *line)
{
    GtkWidget *dialog, *dlg_box, *vbox, *t_entry, *label;
    const char *entry_text;
    char *query=NULL;
    int loop=1, result, dlg_height;
    GString *text;

    dialog = gtk_dialog_new_with_buttons(gettext("Enter web query string"),
                                         GTK_WINDOW(ui_get_mainwindow()),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         NULL);

    dlg_height = 100 + (c->active_fields * 30);
    gtk_widget_set_size_request(dialog, 420, dlg_height);
    dlg_box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    vbox = gtk_vbox_new(FALSE, 3);
    text = create_label_text(c);
    label = gtk_label_new(text->str);
    t_entry = gtk_entry_new();

    gtk_entry_set_text(GTK_ENTRY(t_entry), "$1 $name");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), t_entry, FALSE, FALSE, 0);

    /* run dialog */
    gtk_container_add(GTK_CONTAINER(dlg_box), vbox);
    gtk_widget_show_all(dialog);
    ui_prepend_mainwindow(dialog);

    do {
        result = gtk_dialog_run(GTK_DIALOG(dialog));

        if (result == GTK_RESPONSE_ACCEPT) {
            entry_text = gtk_entry_get_text(GTK_ENTRY(t_entry));
            query = parse_text_entry(entry_text, c, line);

            if (query != NULL)
                loop = 0;
        } else
            loop = 0;
    } while (loop);

    g_string_free(text, TRUE);
    ui_remove_mainwindow(dialog);
    gtk_widget_destroy(dialog);

    return query;
}

static void move_image_to_tmp_dir(const char *old, const char *new)
{
    mode_t mode;

    mode = S_IWUSR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if (access(UNSAVED_IMG_TMP_DIR, 0x00) == -1)
        mkdir(UNSAVED_IMG_TMP_DIR, mode);

    rename_file(old, new);
}

static char *get_cover_image_from_web(struct db_collection *c,
    struct dlg_line *line)
{
    char *query=NULL, *filename=NULL, *s;

    if (access(IMAGE_PLUGIN, 0x00) == -1) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Image plugin not found. Check your installation!"));

        return NULL;
    }

    query = web_query_dlg(c, line);

    if (query == NULL)
        return NULL;

    filename = web_cover_dlg(query);

    if (filename != NULL) {
        s = create_new_filename();
        move_image_to_tmp_dir(filename, s);
        g_free(filename);

        return s;
    }

    return NULL;
}

static char *resize_image(const char *filename)
{
    char *resized_filename;
    pid_t pid;
    int c_status;
    const char *args[] = {
        "/usr/bin/convert", "convert", "-resize", "150x150", NULL
    };

    resized_filename = create_new_filename();
    pid = fork();

    if (pid < 0) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error resizing image to temporary file."));

        return NULL;
    } else if (pid == 0) {
        execl(args[0], args[1], args[2], args[3], filename, resized_filename,
              (char *)NULL);

        _exit(0);
    } else
        waitpid(pid, &c_status, WUNTRACED);

    return resized_filename;
}

static char *get_cover_image_local(void)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    char *c=NULL, *filename=NULL;

    dialog = gtk_file_chooser_dialog_new(gettext("Select image file"),
                                         GTK_WINDOW(ui_get_mainwindow()),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "JPG files");
    gtk_file_filter_add_pattern(filter, "*.jpg");

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        c = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (c != NULL) {
            /* resize image to the supported size (150x150) */
            filename = resize_image(c);
            free(c);
        }
    }

    gtk_widget_destroy(dialog);
    return filename;
}

static int has_data_to_search(struct dlg_line *line)
{
    GList *l;

    for (l = g_list_first(line->column); l; l = l->next)
        if (!strlen((const char *)l->data))
            return 0;

    return 1;
}

char *get_cover_image_file(struct db_collection *c, struct dlg_line *line)
{
    GtkWidget *dialog, *dlg_box, *vbox, *rd_web, *rd_local;
    int result;
    char *filename=NULL;

    dialog = gtk_dialog_new_with_buttons(gettext("Select cover image source"),
                                         GTK_WINDOW(ui_get_mainwindow()),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         NULL);

    gtk_widget_set_size_request(dialog, 400, 100);
    dlg_box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    vbox = gtk_vbox_new(FALSE, 5);

    rd_local = gtk_radio_button_new_with_label(NULL,
                                               gettext("Select image from local disc"));

    rd_web = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rd_local),
                                                         gettext("Search the web for an image"));

    gtk_box_pack_start_defaults(GTK_BOX(vbox), rd_local);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), rd_web);
    gtk_box_pack_start(GTK_BOX(dlg_box), vbox, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rd_local)))
            filename = get_cover_image_local();
        else {
            if (!has_data_to_search(line)) {
                display_msg(GTK_MESSAGE_WARNING, gettext("Attention"),
                            gettext("All fields must be filled to use the web for "
                                    "this search"));

                gtk_widget_destroy(dialog);
                return NULL;
            }

            filename = get_cover_image_from_web(c, line);
        }
    }

    gtk_widget_destroy(dialog);

    return filename;
}

