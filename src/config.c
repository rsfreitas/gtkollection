
/*
 * Description: config file manipulation.
 */

#include <stdlib.h>
#include <string.h>
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

void load_config_file(struct app_settings *settings)
{
    GKeyFile *key_file;
    GKeyFileFlags flags;
    GError *error=NULL;
    char filename[216]={0};

    sprintf(filename, "%s/%s/config/gtkollection.conf", getenv("HOME"),
            APP_CONFIG_PATH);

    key_file = g_key_file_new();
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

    if (!g_key_file_load_from_file(key_file, filename, flags, &error)) {
        load_default_values(settings);
        return;
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

    g_key_file_free(key_file);
}

void save_config_file(struct app_settings settings)
{
    GKeyFile *key_file;
    GError *error=NULL;
    char filename[216]={0};
    gchar *data;
    gsize length;
    FILE *f;

    sprintf(filename, "%s/%s/config/gtkollection.conf", getenv("HOME"),
            APP_CONFIG_PATH);

    key_file = g_key_file_new();

    g_key_file_set_boolean(key_file, "mainwindow", "maximized", settings.maximized);
    g_key_file_set_integer(key_file, "mainwindow", "width", settings.wnd_width);
    g_key_file_set_integer(key_file, "mainwindow", "height", settings.wnd_height);
    g_key_file_set_integer(key_file, "mainwindow", "pos_x", settings.pos_x);
    g_key_file_set_integer(key_file, "mainwindow", "pos_y", settings.pos_y);

    data = g_key_file_to_data(key_file, &length, &error);
    f = fopen(filename, "w+");

    if (!f) {
        fprintf(stderr, gettext("Error saving configuration file!\n"));
        return;
    }

    fwrite(data, 1, length, f);
    fclose(f);

    g_free(data);
    g_key_file_free(key_file);
}

