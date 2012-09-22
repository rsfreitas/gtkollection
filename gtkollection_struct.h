
#ifndef _GTKOLLECTION_STRUCT_H
#define _GTKOLLECTION_STRUCT_H			1

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

#endif

