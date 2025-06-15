#include "result_dialog.h"

ResultAction show_result_dialog(GtkWindow *parent, gboolean is_win) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Game Result",
        parent,
        GTK_DIALOG_MODAL,
        "_Restart", RESULT_RESTART,
        "_Change Mode", RESULT_MODE_SELECT,
        "_Quit", RESULT_QUIT,
        NULL
    );

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new(NULL);

    if (is_win) {
        gtk_label_set_markup(GTK_LABEL(label),
            "<span foreground='red' font_desc='20.0'>🎉 You Win!</span>");
    } else {
        gtk_label_set_markup(GTK_LABEL(label),
            "<span foreground='blue' font_desc='20.0'>😢 You Lose...</span>");
    }

    gtk_container_add(GTK_CONTAINER(content_area), label);
    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return (ResultAction)response;
}
