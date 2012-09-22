
/*
 * Description: diverse functions.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <libintl.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "gtkollection.h"

#define VALID_CHARS "1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM"

static GList *__active_windows = NULL;

GtkWidget *ui_get_mainwindow(void)
{
    GList *l;
    GtkWidget *w;

    l = g_list_first(__active_windows);
    w = (GtkWidget *)l->data;

    return w;
}

void ui_prepend_mainwindow(GtkWidget *w)
{
    __active_windows = g_list_prepend(__active_windows, w);
}

void ui_remove_mainwindow(GtkWidget *w)
{
    __active_windows = g_list_remove(__active_windows, w);
}

struct db_collection *create_db_collection(const char *screen_name, const char *name,
    int id)
{
    struct db_collection *c;

    c = malloc(sizeof(struct db_collection));

    if (!c)
        return NULL;

    c->screen_name = strdup(screen_name);

    if (name == NULL)
        c->name = screen_name_to_name(screen_name);
    else
        c->name = strdup(name);

    c->id = id;
    c->active_fields = 0;
    c->n_fields = 0;
    c->n_entries = 0;
    c->fields = NULL;
    c->sql_fields_stmt = g_string_new(NULL);
    c->image_path = NULL;

    return c;
}

void destroy_db_collection(struct db_collection *c,
    gpointer user_data __attribute__((unused)))
{
    if (c->image_path != NULL)
        free(c->image_path);

    g_list_free_full(c->fields, (GDestroyNotify)destroy_db_field);
    g_string_free(c->sql_fields_stmt, TRUE);
    free(c->screen_name);
    free(c->name);
    free(c);
}

void add_db_field(struct db_collection *c, struct db_field *f)
{
    c->n_fields++;

    if (f->status == FIELD_ACTIVE) {
        c->active_fields++;
        g_string_append_printf(c->sql_fields_stmt, "%s, ", f->name);
    }

    c->fields = g_list_append(c->fields, f);
}

void end_add_db_field(struct db_collection *c)
{
    g_string_erase(c->sql_fields_stmt, c->sql_fields_stmt->len - 2, -1);
}

struct db_field *create_db_field(const char *name, const char *screen_name,
    int idx, int field_status)
{
    struct db_field *f;

    f = malloc(sizeof(struct db_field));

    if (!f)
        return NULL;

    f->name = strdup(name);
    f->screen_name = strdup(screen_name);
    f->idx = idx;
    f->status = field_status;

    return f;
}

void destroy_db_field(struct db_field *f)
{
    free(f->screen_name);
    free(f->name);
    free(f);
}

void display_msg(GtkMessageType msg_type, const char *title, const char *fmt, ...)
{
    GtkWidget *dialog;
    va_list ap;
    char str[128]={0};

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    dialog = gtk_message_dialog_new(GTK_WINDOW(ui_get_mainwindow()),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    msg_type, GTK_BUTTONS_OK, str, NULL);

    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

char *screen_name_to_name(const char *sn)
{
    char *s;

    s = g_ascii_strdown(sn, strlen(sn));

    return g_strcanon(s, VALID_CHARS, '_');
}

struct dlg_data *create_dlg_data(void)
{
    struct dlg_data *dlg_data;

    dlg_data = malloc(sizeof(struct dlg_data));

    if (!dlg_data)
        return NULL;

    dlg_data->priv.d_lines = NULL;
    dlg_data->priv.data = g_array_new(FALSE, FALSE, sizeof(struct dlg_line));
    dlg_data->priv.bt_img_filename = NULL;

    return dlg_data;
}

struct dlg_line *create_dlg_line(int line_status)
{
    struct dlg_line *line;

    line = malloc(sizeof(struct dlg_line));

    if (!line)
        return NULL;

    line->column = NULL;
    line->new_column = NULL;
    line->img_filename = NULL;
    line->status = line_status;

    return line;
}

void destroy_dlg_line(struct dlg_line *line)
{
    if (line->new_column != NULL) {
        g_list_free(line->new_column);
        line->new_column = NULL;
    }

    if (line->column != NULL) {
        g_list_free(line->column);
        line->column = NULL;
    }
}

int choose_msg(const char *title, const char *fmt, ...)
{
    GtkWidget *dialog;
    va_list ap;
    char str[128]={0};
    int result;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    dialog = gtk_message_dialog_new(GTK_WINDOW(ui_get_mainwindow()),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO, str, NULL);

    gtk_window_set_title(GTK_WINDOW(dialog), title);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_YES) ? 1 : 0;
}

int init_gettext(char *pPackage, char *pDirectory)
{
    if (!setlocale(LC_ALL, ""))
        return 0;

    if (!setlocale(LC_NUMERIC, "en_US.UTF-8"))
        return 0;

    if (!bindtextdomain(pPackage, pDirectory))
        return 0;

    if (!textdomain(pPackage))
        return 0;

    return 1;
}

#define MY_MAXSIZE ((gsize) - 1)

static inline gsize nearest_power(gsize base, gsize num)
{
    if (num > MY_MAXSIZE / 2)
        return MY_MAXSIZE;
    else {
        gsize n = base;

        while (n < num)
            n <<= 1;

        return n;
    }
}

static void g_string_maybe_expand(GString *string, gint len)
{
    if (string->len + len >= string->allocated_len) {
        string->allocated_len = nearest_power(1, string->len + len + 1);
        string->str = g_realloc(string->str, string->allocated_len);
    }
}

/**
 * g_string_replace:
 * @string: a #GString
 * @sub: the substring target in #GString, to be replaced.
 * @repl: the third string to replace sub.
 *
 * Replaces all instances of @sub from a #GString with @repl.
 * if @sub is empty or no instance found in #GString, nothing will happen.
 * if @repl is empty, it will erase all @sub (s) in #GString.
 *
 * Returns: @string
 */
GString* g_string_replace(GString *string, const gchar *sub, const gchar *repl)
{
    gsize ls, lr;
    gchar *q, *t;

    g_return_val_if_fail(string != NULL, NULL);
    g_return_val_if_fail(sub != NULL, NULL);
    g_return_val_if_fail(repl != NULL, NULL);

    q = string->str;
    ls = strlen(sub);
    lr = strlen(repl);

    if (!ls)
        goto end;

    if (ls > lr) {
        while (*q != '\0' && (t = strstr(q, sub)) != NULL) {
            memcpy(t, repl, lr);
            strcpy(t + lr, t + ls);
            q = t + lr;
            string->len -= ls - lr;
        }
    } else if (ls == lr) {
        while (*q != '\0' && (t = strstr(q, sub)) != NULL) {
            memcpy(t, repl, lr);
            q = t + lr;
        }
    } else {
        while(*q != '\0' && (t = strstr(q, sub)) != NULL) {
            ptrdiff_t offt, offq;

            offq = q - string->str;
            offt = t - string->str;
            g_string_maybe_expand(string, lr - ls);
            q = string->str + offq;
            t = string->str + offt;
            g_memmove(t + lr, t + ls, strlen(t + ls) + 1);
            memcpy(t, repl, lr);
            q = t + lr;
            string->len += lr - ls;
        }
    }

end:
    return string;
}

char *strrand(int size)
{
    int i, n;
    char *s;

    s = calloc(1, size * sizeof(char));

    for (i = 0; i < size; i++) {
        n = (int)(26.0 * (rand() / (RAND_MAX + 1.0)));
        n += 'a';
        s[i] = (char)n;
    }

    return s;
}

void rename_file(const char *old, const char *new)
{
    pid_t pid;
    int c_status;
    const char *args[] = {
        "/bin/mv", "mv", "-f", NULL
    };

    pid = fork();

    if (pid < 0) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error renaming image to temporary name."));

        return;
    } else if (pid == 0) {
        execl(args[0], args[1], args[2], old, new, (char *)NULL);
        _exit(0);
    } else
        waitpid(pid, &c_status, WUNTRACED);
}

void remove_collection_dir(int collection_id)
{
    pid_t pid;
    int c_status;
    char path[256]={0};
    const char *args[] = {
        "/bin/rm", "rm", "-rf", NULL
    };

    pid = fork();
    snprintf(path, sizeof(path), "%s/%s/collections/c%d", getenv("HOME"),
             APP_CONFIG_PATH, collection_id);

    if (pid < 0) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error removing collection directory."));

        return;
    } else if (pid == 0) {
        execl(args[0], args[1], args[2], path, (char *)NULL);
        _exit(0);
    } else
        waitpid(pid, &c_status, WUNTRACED);
}

void create_unsaved_images_tmp_dir(void)
{
    mode_t mode;
    pid_t pid;
    int c_status;
    const char *args[] = {
        "/bin/rm", "rm", "-rf", NULL
    };

    mode = S_IWUSR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if (!access(UNSAVED_IMG_TMP_DIR, 0x00)) {
        pid = fork();

        if (pid < 0) {
            fprintf(stderr, gettext("Error while removing temporary directory.\n"));
            return;
        } else if (pid == 0) {
            execl(args[0], args[1], args[2], UNSAVED_IMG_TMP_DIR, (char *)NULL);
            _exit(0);
        } else
            waitpid(pid, &c_status, WUNTRACED);
    }

    mkdir(UNSAVED_IMG_TMP_DIR, mode);
}

char *load_license_file(void)
{
    FILE *f;
    char *b;
    long fsize;

    f = fopen(LICENSE_FILE, "r");

    if (!f)
        return NULL;

    fseek(f, 0L, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    b = calloc(1, (fsize + 1) * sizeof(char));

    if (!b) {
        fclose(f);
        return NULL;
    }

    fread(b, fsize, 1, f);
    fclose(f);

    return b;
}

