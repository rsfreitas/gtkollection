
/*
 * Description: config file manipulation.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libintl.h>

#include "gtkollection.h"

int access_app_config_dir(void)
{
    char path[128]={0};

    snprintf(path, 128, "%s/%s", getenv("HOME"), APP_CONFIG_PATH);

    if (access(path, 0x00) == -1)
        return 0;

    return 1;
}

void create_app_config_dir(void)
{
    char path[128]={0}, s_path[160];
    const char *subdir[] = { "config", "database", "collections" };
    int t_sub, i;
    mode_t mode;

    mode = S_IWUSR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    t_sub = (sizeof subdir / sizeof subdir[0]);
    snprintf(path, 128, "%s/%s", getenv("HOME"), APP_CONFIG_PATH);
    mkdir(path, mode);

    for (i = 0; i < t_sub; i++) {
        memset(s_path, 0, sizeof(s_path));
        snprintf(s_path, 160, "%s/%s", path, subdir[i]);
        mkdir(s_path, mode);
    }
}

static void load_default_values(struct app_settings *settings)
{
    settings->maximized = FALSE;

    settings->wnd_width = 800;
    settings->wnd_height = 600;

    settings->pos_x = -1;
    settings->pos_y = -1;
}

static char *get_config_filename(void)
{
    char filename[256]={0};

    sprintf(filename, "%s/%s/config/gtkollection.conf", getenv("HOME"),
            APP_CONFIG_PATH);

    return strdup(filename);
}

void load_config_file(struct app_settings *settings)
{
    GKeyFile *key_file;
    GKeyFileFlags flags;
    GError *error=NULL;
    char *filename;
    int default_values=0;

    filename = get_config_filename();
    key_file = g_key_file_new();
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

    if (!g_key_file_load_from_file(key_file, filename, flags, &error)) {
        g_error_free(error);
        default_values = 1;
        goto end_block;
    }

    if (!g_key_file_has_group(key_file, "mainwindow")) {
        default_values = 1;
        goto end_block;
    }

    settings->maximized = g_key_file_get_boolean(key_file, "mainwindow",
                                                 "maximized", &error);

    settings->wnd_width = g_key_file_get_integer(key_file, "mainwindow",
                                                 "width", &error);

    settings->wnd_height = g_key_file_get_integer(key_file, "mainwindow",
                                                  "height", &error);

    settings->pos_x = g_key_file_get_integer(key_file, "mainwindow", "pos_x",
                                             &error);

    settings->pos_y = g_key_file_get_integer(key_file, "mainwindow", "pos_y",
                                             &error);

end_block:
    settings->sort_info = NULL;
    g_key_file_free(key_file);
    free(filename);

    if (default_values)
        load_default_values(settings);
}

static void save_collection_sort_info(struct app_settings settings,
    GKeyFile *key_file)
{
    GList *l;
    struct collection_sort_info *i;

    for (l = g_list_first(settings.sort_info); l; l = l->next) {
        i = (struct collection_sort_info *)l->data;
        g_key_file_set_integer(key_file, i->name, "enable", i->enable);
        g_key_file_set_integer(key_file, i->name, "idx_field", i->idx_field);
        g_key_file_set_integer(key_file, i->name, "order", i->order);

        free(i->name);
    }

    g_list_free(settings.sort_info);
}

void save_config_file(struct app_settings settings)
{
    GKeyFile *key_file;
    GError *error=NULL;
    char *filename;
    gchar *data;
    gsize length;
    FILE *f;

    filename = get_config_filename();
    key_file = g_key_file_new();

    g_key_file_set_boolean(key_file, "mainwindow", "maximized", settings.maximized);
    g_key_file_set_integer(key_file, "mainwindow", "width", settings.wnd_width);
    g_key_file_set_integer(key_file, "mainwindow", "height", settings.wnd_height);
    g_key_file_set_integer(key_file, "mainwindow", "pos_x", settings.pos_x);
    g_key_file_set_integer(key_file, "mainwindow", "pos_y", settings.pos_y);

    /* save collections sort information */
    save_collection_sort_info(settings, key_file);

    data = g_key_file_to_data(key_file, &length, &error);
    f = fopen(filename, "w+");

    if (!f) {
        fprintf(stderr, gettext("Error saving configuration file!\n"));
        goto end_block;
    }

    fwrite(data, 1, length, f);
    fclose(f);

end_block:
    g_free(data);
    g_key_file_free(key_file);
    free(filename);
}

static void load_collection_info_default_values(struct collection_sort_info *info)
{
    info->enable = FALSE;
    info->idx_field = 0;
    info->order = CONFIG_SORT_ASC;
}

void load_collection_info_from_config(const char *name,
    struct collection_sort_info *info)
{
    char *filename;
    GKeyFile *key_file;
    GKeyFileFlags flags;
    GError *error=NULL;
    int default_values=0;

    filename = get_config_filename();
    key_file = g_key_file_new();
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

    if (!g_key_file_load_from_file(key_file, filename, flags, &error)) {
        g_error_free(error);
        default_values = 1;
        goto end_block;
    }

    if (!g_key_file_has_group(key_file, name)) {
        default_values = 1;
        goto end_block;
    }

    info->enable = g_key_file_get_integer(key_file, name, "enable", &error);
    info->idx_field = g_key_file_get_integer(key_file, name, "idx_field", &error);
    info->order = g_key_file_get_integer(key_file, name, "order", &error);

end_block:
    g_key_file_free(key_file);
    free(filename);

    if (default_values)
        load_collection_info_default_values(info);
}

