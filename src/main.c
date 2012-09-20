
/*
 * Description: main function.
 */

#include <stdlib.h>
#include <libintl.h>

#include "gtkollection.h"

int main(int argc, char **argv)
{
    struct app_settings settings;

    init_gettext(APP_NAME, "");
    srand(time(NULL));
    create_unsaved_images_tmp_dir();

    if (!access_app_config_dir())
        create_app_config_dir();

    load_config_file(&settings);

    if (!db_init()) {
        fprintf(stderr, gettext("Error initialing database.\n"));
        return -1;
    }

    init_ui(&argc, &argv, &settings);
    run_ui(&settings);
    exit_ui(&settings);
    db_uninit();

    save_config_file(settings);
    return 0;
}

