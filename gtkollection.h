
#ifndef _GTKOLLECTION_H
#define _GTKOLLECTION_H			1

#include <gtk/gtk.h>

#define VERSION                         "0.1"

#define APP_NAME                        "gtkollection"
#define APP_CONFIG_PATH                 ".gtkollection"
#define DB_FILENAME                     "collections.db"
#define WEB_IMG_TMP_DIR                 "/tmp/gtkollection_web"
#define UNSAVED_IMG_TMP_DIR             "/tmp/gtkollection_unsaved"
#define IMAGE_PLUGIN                    "/opt/gtkollection/plugins/pl_images"
#define LICENSE_FILE                    "/opt/gtkollection/gpl-2.0.txt"

#define DLG_ADD_ENTRY                   1
#define DLG_UPDATE_ENTRY                2

#define FIELD_ACTIVE                    0
#define FIELD_HIDDEN                    !(FIELD_ACTIVE)

#define CONFIG_SORT_ASC                 0
#define CONFIG_SORT_DESC                1

enum line_status {
    LINE_LOADED = 0,
    LINE_ADDED,
    LINE_UPDATED,
    LINE_DELETED
};

struct collection_sort_info {
    int         enable;
    int         idx_field;
    int         order;
    char        *name;
};

struct app_settings {
    gboolean    maximized;
    int         wnd_width;
    int         wnd_height;
    int         pos_x;
    int         pos_y;
    GList       *sort_info;
};

struct db_field {
    char    *name;
    char    *screen_name;
    int     status;
    int     idx;
};

struct db_collection {
    char    *name;
    char    *screen_name;
    int     id;
    int     active_fields;  /* number of active fields */
    int     n_fields;       /* total number of fields (active/hidden) */
    int     n_entries;
    GString *sql_fields_stmt;
    GList   *fields;
    char    *image_path;
};

struct dlg_line {
    int                 status;
    GList               *column;
    GList               *new_column;
    char                *img_filename;
    unsigned long long  id;
};

typedef struct dlg_line dlg_line;

struct private_dlg_data {
    /* widgets */
    GtkWidget   *treeview;
    GtkWidget   **textbox;
    GtkWidget   *bt_img;
    GtkWidget   *image;
    GtkWidget   *check_bt_sort;
    GtkWidget   *sort_combo;
    GtkWidget   *rd_asc;
    GtkWidget   *rd_desc;

    /* data */
    GtkTreeModel    *model;
    int             tab_column_idx;
    char            *bt_img_filename;

    /* model data (array of dlg_line structures) */
    GArray          *data;

    /* deleted lines */
    GList           *d_lines;
};

struct dlg_data {
    /* Internal notebook tab widgets and data */
    struct private_dlg_data priv;

    GtkWidget               *notebook;
    GtkWidget               *page;
    GtkWidget               *bt_save;
    struct db_collection    *c;

    /* collection sort configuration */
    struct collection_sort_info info;
};

/* config.c */
int access_app_config_dir(void);
void create_app_config_dir(void);
void load_config_file(struct app_settings *settings);
void save_config_file(struct app_settings settings);
void load_collection_info_from_config(const char *name,
                                      struct collection_sort_info *info);

/* common.c */
struct db_collection *create_db_collection(const char *screen_name, const char *name,
                                           int id);

void destroy_db_collection(struct db_collection *c,
                           gpointer user_data __attribute__((unused)));

void add_db_field(struct db_collection *c, struct db_field *f);
void end_add_db_field(struct db_collection *c);
void destroy_db_field(struct db_field *f);
struct db_field *create_db_field(const char *name, const char *screen_name,
                                 int idx, int field_status);

void display_msg(GtkMessageType msg_type, const char *title, const char *fmt, ...);
int choose_msg(const char *title, const char *fmt, ...);

char *screen_name_to_name(const char *sn);
struct dlg_data *create_dlg_data(void);
struct dlg_line *create_dlg_line(int line_status);
void destroy_dlg_line(struct dlg_line *line);

GtkWidget *ui_get_mainwindow(void);
void ui_prepend_mainwindow(GtkWidget *w);
void ui_remove_mainwindow(GtkWidget *w);

int init_gettext(char *pPackage, char *pDirectory);
GString* g_string_replace(GString *string, const gchar *sub, const gchar *repl);
char *strrand(int size);
void rename_file(const char *old, const char *new);
void create_unsaved_images_tmp_dir(void);
void remove_collection_dir(int collection_id);
char *load_license_file(void);

/* gtk_gui.c */
void init_ui(int *argcp, char ***argvp, struct app_settings *settings);
void exit_ui(struct app_settings *settings);
void run_ui(struct app_settings *settings);

/* database.c */
int db_init(void);
void db_uninit(void);

void db_create_collection(struct db_collection *c, int gtk_status);
int db_delete_collection(const char *name);
int db_update_collection(struct db_collection *new_c, struct db_collection *original_c);

GList *db_get_all_collection_info(void);

int db_delete_collection_data(struct db_collection *c, GList *entries);
int db_update_collection_data(struct db_collection *c, GArray *entries);
void db_load_and_set_collection_data(struct db_collection *c, GtkListStore *store,
                                     struct dlg_data *dlg_data);

/* collection_notebook.c */
struct dlg_data *collection_widget(struct db_collection *c, GtkWidget *notebook);

/* collection_dialog.c */
struct db_collection *do_add_dialog(GtkWidget *main_window, struct db_collection *db);

/* image_dialog.c */
char *get_cover_image_file(struct db_collection *c, struct dlg_line *line);

#endif

