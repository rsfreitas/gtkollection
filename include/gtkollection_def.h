
#ifndef _GTKOLLECTION_DEF_H
#define _GTKOLLECTION_DEF_H             1

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

#endif

