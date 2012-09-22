
/*
 * Description: sqlite database manipulation.
 */

#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>

#include <sqlite3.h>

#include "gtkollection.h"

#define SEARCH_ONLY_NAME                1
#define SEARCH_STATUS_DIFF              2

static sqlite3 *__db;

static int search_in_list(GList *list, struct db_field *f, int search_type)
{
    GList *l;
    struct db_field *lf;

    for (l = g_list_first(list); l; l = l->next) {
        lf = (struct db_field *)l->data;

        switch (search_type) {
            case SEARCH_ONLY_NAME:
                if (!strcmp(lf->name, f->name))
                    return 1;

                break;

            case SEARCH_STATUS_DIFF:
                if (!strcmp(lf->name, f->name) &&
                    (lf->status != f->status))
                {
                    return 1;
                }

                break;
        }
    }

    return 0;
}

static GList *fields_compare(GList *l1, GList *l2)
{
    GList *diff=NULL, *l;
    struct db_field *f;

    for (l = g_list_first(l1); l; l = l->next) {
        f = (struct db_field *)l->data;

        if (!search_in_list(l2, f, SEARCH_ONLY_NAME))
            diff = g_list_append(diff, f);
    }

    return diff;
}

static GList *fields_compare_by_status(GList *l1, GList *l2)
{
    GList *diff=NULL, *l;
    struct db_field *f;

    for (l = g_list_first(l1); l; l = l->next) {
        f = (struct db_field *)l->data;

        if (search_in_list(l2, f, SEARCH_STATUS_DIFF))
            diff = g_list_append(diff, f);
    }

    return diff;
}

static int db_create_main_tables(void)
{
    char *emsg=NULL;

    if (sqlite3_exec(__db, "CREATE TABLE tab_collection ("
                           "id integer primary key autoincrement, "
                           "name varchar(256) NOT NULL, "
                           "screen_name varchar(256) NOT NULL"
                           ")",
                           NULL, 0, &emsg) != SQLITE_OK)
    {
        fprintf(stderr, "Error: %s\n", emsg);
        sqlite3_free(emsg);
        return 0;
    }

    if (sqlite3_exec(__db, "CREATE TABLE collection_fields ("
                           "cat_id int(5) NOT NULL, "
                           "name varchar(256) NOT NULL, "
                           "field_size int(4) NOT NULL, "
                           "screen_name varchar(256) NOT NULL, "
                           "status int(4) NOT NULL"
                           ")",
                           NULL, 0, &emsg) != SQLITE_OK)
    {
        fprintf(stderr, "Error: %s\n", emsg);
        sqlite3_free(emsg);
        return 0;
    }

    return 1;
}

static int db_get_collection_id(const char *name)
{
    int id=0;
    char str_query[256]={0};
    sqlite3_stmt *stmt;

    snprintf(str_query, 256, "SELECT id FROM tab_collection WHERE name = '%s'",
             name);

    if (sqlite3_prepare_v2(__db, str_query, -1, &stmt, NULL) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext(gettext("Error")),
                    gettext("Error searching the '%s' collection id"), name);

        return -1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
        id = strtol((char *)sqlite3_column_text(stmt, 0), NULL, 10);

    sqlite3_finalize(stmt);

    return id;
}

#define DEFAULT_FIELD_SIZE                  256

static int db_create_collection_table(struct db_collection *c)
{
    char *emsg;
    GString *s;
    GList *l;
    struct db_field *f;

    s = g_string_new(NULL);
    g_string_printf(s, "CREATE TABLE %s ("
                       "id integer primary key autoincrement, "
                       "c_image varchar(256) default default_image_xpm",
                       c->name);

    for (l = g_list_first(c->fields); l; l = l->next) {
        f = (struct db_field *)l->data;
        g_string_append_printf(s, ", %s varchar(%d)", f->name,
                               DEFAULT_FIELD_SIZE);
    }

    g_string_append_printf(s, ")");

    if (sqlite3_exec(__db, s->str, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext(gettext("Error")), "%s", emsg);
        sqlite3_free(emsg);
        return -1;
    }

    g_string_free(s, TRUE);
    return 0;
}

static void __insert_field(struct db_field *f, int *collection_id)
{
    char str_query[256]={0}, *emsg;

    memset(str_query, 0, sizeof(str_query));
    snprintf(str_query, 256, "INSERT INTO collection_fields (cat_id, name, "
                             "field_size, screen_name, status) VALUES "
                             "(%d, \"%s\", %d, \"%s\", %d)",
                             *collection_id, f->name, DEFAULT_FIELD_SIZE,
                             f->screen_name, f->status);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return;
    }
}

static char *get_db_collection_image_path(int collection_id)
{
    char path[256]={0};

    snprintf(path, sizeof(path), "%s/%s/collections/c%d", getenv("HOME"),
             APP_CONFIG_PATH, collection_id);

    return strdup(path);
}

static void create_collection_image_dir(int collection_id)
{
    char *path;
    mode_t mode;

    mode = S_IWUSR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    path = get_db_collection_image_path(collection_id);

    if (access(path, 0x00) == -1)
        mkdir(path, mode);

    free(path);
}

void db_create_collection(struct db_collection *c, int gtk_status)
{
    char str_query[256]={0}, *emsg;
    int collection_id;

    if (db_create_collection_table(c) < 0)
        return;

    /* Insert the new collection entry */
    snprintf(str_query, 256, "INSERT INTO tab_collection (name, screen_name) "
                             "VALUES (\"%s\", \"%s\")", c->name, c->screen_name);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        if (gtk_status == TRUE)
            display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        else
            fprintf(stderr, "Error: %s\n", emsg);

        sqlite3_free(emsg);
        return;
    }

    collection_id = db_get_collection_id(c->name);

    /* Insert fields from the new collection */
    g_list_foreach(c->fields, (GFunc)__insert_field, &collection_id);

    create_collection_image_dir(collection_id);
    c->image_path = get_db_collection_image_path(collection_id);
}

int db_delete_collection(const char *name)
{
    char str_query[256]={0}, *emsg;
    int collection_id;

    collection_id = db_get_collection_id(name);

    if (collection_id <= 0) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Collection '%s' not found!"), name);

        return 0;
    }

    snprintf(str_query, 256, "DELETE FROM tab_collection WHERE id = %d",
             collection_id);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return 0;
    }

    snprintf(str_query, 256, "DELETE FROM collection_fields WHERE cat_id = %d",
             collection_id);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return 0;
    }

    snprintf(str_query, 256, "DROP TABLE IF EXISTS %s", name);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return 0;
    }

    remove_collection_dir(collection_id);
    return 1;
}

static void db_add_field_column(struct db_field *f, struct db_collection *c)
{
    char str_query[512]={0}, *emsg;

    snprintf(str_query, 512, "ALTER TABLE %s ADD COLUMN %s varchar(%d)",
             c->name, f->name, DEFAULT_FIELD_SIZE);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return;
    }

    snprintf(str_query, 512, "INSERT INTO collection_fields (cat_id, name, "
                             "field_size, screen_name, status) VALUES "
                             "(%d, \"%s\", %d, \"%s\", %d)",
                             c->id, f->name, DEFAULT_FIELD_SIZE, f->screen_name,
                             FIELD_ACTIVE);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return;
    }
}

static void db_update_field_status(struct db_field *f, struct db_collection *c)
{
    char str_query[512]={0}, *emsg;

    snprintf(str_query, 512, "UPDATE collection_fields SET status = %d WHERE "
                             "cat_id = %d AND name = \"%s\"", !f->status,
                             c->id, f->name);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return;
    }
}

static void db_change_collection_name(struct db_collection *original,
    struct db_collection *new)
{
    char str_query[512]={0}, *emsg;

    snprintf(str_query, 512, "ALTER TABLE %s RENAME TO %s",
             original->name, new->name);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return;
    }

    snprintf(str_query, 512, "UPDATE tab_collection SET name = \"%s\", "
                             "screen_name = \"%s\" WHERE id = %d",
                             new->name, new->screen_name, original->id);

    if (sqlite3_exec(__db, str_query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return;
    }
}

static void reload_sql_fields_stmt(struct db_collection *c)
{
    GList *l;
    struct db_field *f;
    int active_fields=0;

    g_string_free(c->sql_fields_stmt, TRUE);
    c->sql_fields_stmt = g_string_new(NULL);

    for (l = g_list_first(c->fields); l; l = l->next) {
        f = (struct db_field *)l->data;

        if (f->status == FIELD_ACTIVE) {
            active_fields++;
            g_string_append_printf(c->sql_fields_stmt, "%s, ", f->name);
        }
    }

    end_add_db_field(c);

    /* also update active_fields info */
    c->active_fields = active_fields;
}

int db_update_collection(struct db_collection *new_c, struct db_collection *original_c)
{
    GList *l_added, *l_status;
    int updated=0;

    l_added = fields_compare(new_c->fields, original_c->fields);
    l_status = fields_compare_by_status(original_c->fields, new_c->fields);

    /* if any field status needs to be updated */
    if (l_status)
        g_list_foreach(l_status, (GFunc)db_update_field_status, original_c);

    /* uses the same id from the original collection */
    original_c->id = db_get_collection_id(original_c->name);
    new_c->id = original_c->id;

    /* if a field has been added */
    if (g_list_length(l_added))
        g_list_foreach(l_added, (GFunc)db_add_field_column, original_c);

    /* if the collection's name changed */
    if (strcmp(original_c->name, new_c->name)) {
        db_change_collection_name(original_c, new_c);
        updated = 1;
    }

    reload_sql_fields_stmt(new_c);

    /* also get the number of entries from the original collection */
    new_c->n_entries = original_c->n_entries;

    return ((l_added != NULL) || (l_status != NULL) || updated);
}

static void create_default_collections(void)
{
    struct db_collection *c;
    struct db_field *f;

    /* CD */
    c = create_db_collection("CD", NULL, 0);

    f = create_db_field("artista", "Artista", 0, FIELD_ACTIVE);
    add_db_field(c, f);

    f = create_db_field("album", "Album", 1, FIELD_ACTIVE);
    add_db_field(c, f);

    f = create_db_field("genero", "Genero", 2, FIELD_ACTIVE);
    add_db_field(c, f);

    db_create_collection(c, FALSE);
    destroy_db_collection(c, NULL);

    /* DVD */
    c = create_db_collection("DVD", NULL, 0);

    f = create_db_field("titulo", "Titulo", 0, FIELD_ACTIVE);
    add_db_field(c, f);

    f = create_db_field("diretor", "Diretor", 1, FIELD_ACTIVE);
    add_db_field(c, f);

    f = create_db_field("genero", "Genero", 2, FIELD_ACTIVE);
    add_db_field(c, f);

    db_create_collection(c, FALSE);
    destroy_db_collection(c, NULL);
}

static void db_load_fields_from_collection(struct db_collection *c)
{
    char str_query[256]={0}, name[256]={0}, screen_name[256]={0};
    int idx=0, status;
    sqlite3_stmt *stmt;
    struct db_field *f;

    snprintf(str_query, 256, "SELECT name, screen_name, status "
                             "FROM collection_fields "
                             "WHERE cat_id = %d", c->id);

    if (sqlite3_prepare_v2(__db, str_query, -1, &stmt, NULL) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error searching for the '%s' collection fields"),
                    c->name);

        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        memset(name, 0, sizeof(name));
        strcpy(name, (char *)sqlite3_column_text(stmt, 0));

        memset(screen_name, 0, sizeof(screen_name));
        strcpy(screen_name, (char *)sqlite3_column_text(stmt, 1));

        status = strtoll((char *)sqlite3_column_text(stmt, 2), NULL, 10);

        f = create_db_field(name, screen_name, idx, status);

        if (f == NULL)
            return;

        add_db_field(c, f);
        idx++;
    }

    end_add_db_field(c);
}

static struct db_collection *db_load_collection_info(sqlite3_stmt *stmt)
{
    struct db_collection *c;
    char name[256]={0}, screen_name[256]={0};
    int id;

    id = strtoll((char *)sqlite3_column_text(stmt, 0), NULL, 10);
    strcpy(screen_name, (char *)sqlite3_column_text(stmt, 1));
    strcpy(name, (char *)sqlite3_column_text(stmt, 2));

    c = create_db_collection(screen_name, name, id);
    db_load_fields_from_collection(c);
    c->image_path = get_db_collection_image_path(c->id);

    return c;
}

static void db_load_collection_data(struct db_collection *c)
{
    char str_query[256]={0};
    sqlite3_stmt *stmt;

    snprintf(str_query, 256, "SELECT count(*) FROM %s", c->name);

    if (sqlite3_prepare_v2(__db, str_query, -1, &stmt, NULL) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error searching the '%s' collection data"), c->name);

        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
        c->n_entries = strtol((char *)sqlite3_column_text(stmt, 0), NULL, 10);

    sqlite3_finalize(stmt);
}

GList *db_get_all_collection_info(void)
{
    GList *l_db=NULL;
    char str_query[256]={0};
    sqlite3_stmt *stmt;
    struct db_collection *c_db;

    snprintf(str_query, 256, "SELECT id, screen_name, name FROM tab_collection");

    if (sqlite3_prepare_v2(__db, str_query, -1, &stmt, NULL) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error searching for collections info"));

        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        c_db = db_load_collection_info(stmt);

        if (c_db == NULL)
            return NULL;

        db_load_collection_data(c_db);
        l_db = g_list_append(l_db, c_db);
    }

    sqlite3_finalize(stmt);

    return l_db;
}

void db_load_and_set_collection_data(struct db_collection *c, GtkListStore *store,
    struct dlg_data *dlg_data)
{
    GtkTreeIter iter;
    char str_query[256]={0}, *column, *s;
    sqlite3_stmt *stmt;
    struct dlg_line *line;
    int i;

    snprintf(str_query, 256, "SELECT c_image, id, %s FROM %s", c->sql_fields_stmt->str,
             c->name);

    if (sqlite3_prepare_v2(__db, str_query, -1, &stmt, NULL) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                    gettext("Error searching the '%s' collection data"), c->name);

        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        gtk_list_store_append(GTK_LIST_STORE(store), &iter);
        line = create_dlg_line(LINE_LOADED);

        if (!line) {
            display_msg(GTK_MESSAGE_ERROR, gettext("Error"),
                        gettext("Error creating new entry!"));

            return;
        }

        line->img_filename = strdup((char *)sqlite3_column_text(stmt, 0));
        line->id = strtol((char *)sqlite3_column_text(stmt, 1), NULL, 10);

        for (i = 0; i < c->active_fields; i++) {
            s = (char *)sqlite3_column_text(stmt, i + 2);

            if (s != NULL)
                column = strdup(s);
            else
                column = strdup("");

            gtk_list_store_set(GTK_LIST_STORE(store), &iter, i, column, -1);
            line->column = g_list_append(line->column, column);
        }

        g_array_append_vals(dlg_data->priv.data, line, 1);
    }

    sqlite3_finalize(stmt);
}

int db_delete_collection_data(struct db_collection *c, GList *entries)
{
    GList *l;
    struct dlg_line *line;
    GString *query;
    char *emsg;
    int deleted=0;

    for (l = g_list_first(entries); l; l = l->next) {
        line = (struct dlg_line *)l->data;
        query = g_string_new(NULL);
        g_string_printf(query, "DELETE FROM %s WHERE id = %llu", c->name, line->id);

        if (sqlite3_exec(__db, query->str, NULL, 0, &emsg) != SQLITE_OK) {
            display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
            sqlite3_free(emsg);
            return 0;
        }

        /* also removes the entry image */
        if (strcmp(line->img_filename, "default_image_xpm"))
            remove(line->img_filename);

        g_string_free(query, TRUE);
        deleted++;
    }

    return deleted;
}

static void dlg_line_replace_data(struct dlg_line *line)
{
    GList *l;
    int p;

    for (l = g_list_first(line->column), p = 0; l; l = l->next, p++) {
        free(l->data);
        l->data = strdup(g_list_nth_data(line->new_column, p));
    }
}

static sqlite3_int64 db_get_last_rowid(void)
{
    sqlite3_int64 id;

    id = sqlite3_last_insert_rowid(__db);

    return id;
}

static void db_update_image_entry_info(struct db_collection *c, struct dlg_line *line)
{
    char *emsg, query[512]={0}, *tmp;
    GString *new_filename;

    tmp = strdup(line->img_filename);
    new_filename = g_string_new(NULL);
    g_string_printf(new_filename, "%s/%s", c->image_path, basename(tmp));

    if (strcmp(line->img_filename, new_filename->str))
        rename_file(line->img_filename, new_filename->str);

    free(tmp);
    snprintf(query, sizeof(query), "UPDATE %s SET c_image = \"%s\" WHERE id = %llu",
             c->name, new_filename->str, line->id);

    if (sqlite3_exec(__db, query, NULL, 0, &emsg) != SQLITE_OK) {
        display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
        sqlite3_free(emsg);
        return;
    }

    free(line->img_filename);
    line->img_filename = strdup(new_filename->str);
    g_string_free(new_filename, TRUE);
}

int db_update_collection_data(struct db_collection *c, GArray *entries)
{
    unsigned int i;
    struct dlg_line *line;
    struct db_field *f;
    GString *query;
    char *emsg;
    GList *l;
    int p, added=0;

    for (i = 0; i < entries->len; i++) {
        query = g_string_new(NULL);
        line = &g_array_index(entries, dlg_line, i);

        if (line->status == LINE_LOADED)
            continue;

        if (line->status == LINE_ADDED) {
            g_string_printf(query, "INSERT INTO %s (c_image, %s) VALUES (\"%s\",",
                            c->name,
                            c->sql_fields_stmt->str,
                            line->img_filename);

            for (l = g_list_first(line->column); l; l = l->next)
                g_string_append_printf(query, "\"%s\",", (char *)l->data);

            g_string_erase(query, query->len - 1, -1);
            g_string_append_printf(query, ")");

            /* count added lines to update the tab label */
            added++;
        } else if (line->status == LINE_UPDATED) {
            g_string_printf(query, "UPDATE %s SET c_image = \"%s\",", c->name,
                            line->img_filename);

            for (l = g_list_first(c->fields), p = 0; l; l = l->next, p++) {
                f = (struct db_field *)l->data;
                g_string_append_printf(query, " %s = \"%s\",", f->name,
                                       (char *)g_list_nth_data(line->new_column, p));
            }

            g_string_erase(query, query->len - 1, -1);
            g_string_append_printf(query, " WHERE");
            g_string_append_printf(query, " id = %llu", line->id);
        }

        if (sqlite3_exec(__db, query->str, NULL, 0, &emsg) != SQLITE_OK) {
            display_msg(GTK_MESSAGE_ERROR, gettext("Error"), "%s", emsg);
            sqlite3_free(emsg);
            return 0;
        }

        if (line->status == LINE_UPDATED)
            dlg_line_replace_data(line);
        else
            /* get the entry database id for future changes */
            line->id = db_get_last_rowid();

        /* update image path */
        if (strcmp(line->img_filename, "default_image_xpm"))
            db_update_image_entry_info(c, line);

        line->status = LINE_LOADED;
        g_string_free(query, TRUE);
    }

    return added;
}

int db_init(void)
{
    char db_filename[256]={0};
    int create_db=0;

    snprintf(db_filename, 256, "%s/%s/database/%s", getenv("HOME"), APP_CONFIG_PATH,
             DB_FILENAME);

    if (access(db_filename, 0x00) == -1)
        create_db = 1;

    if (sqlite3_open(db_filename, &__db)) {
        sqlite3_close(__db);
        return 0;
    }

    if (create_db) {
        if (!db_create_main_tables())
            return 0;

        create_default_collections();
    }

    return 1;
}

void db_uninit(void)
{
    sqlite3_close(__db);
    sqlite3_shutdown();
}

