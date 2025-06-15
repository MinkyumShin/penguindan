#ifndef RESULT_DIALOG_H
#define RESULT_DIALOG_H

#include <gtk/gtk.h>

typedef enum {
    RESULT_RESTART,
    RESULT_QUIT,
    RESULT_MODE_SELECT
} ResultAction;

ResultAction show_result_dialog(GtkWindow *parent, gboolean is_win);

#endif
