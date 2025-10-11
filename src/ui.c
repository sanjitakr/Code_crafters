#include "ui.h"
#include "utils.h"
#include <gtk/gtk.h>

static GtkWidget *text_view;
static GtkWidget *statusbar;

static void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Collaborative Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    text_view = gtk_text_view_new();
    statusbar = gtk_statusbar_new();

    gtk_box_append(GTK_BOX(vbox), text_view);
    gtk_box_append(GTK_BOX(vbox), statusbar);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    gtk_widget_show(window);
}

static GtkApplication *app;

void ui_init(int argc, char **argv) {
    app = gtk_application_new("com.example.collabeditor", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
}

void ui_update_text(const char *text) {
    if (!text_view) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, text, -1);
}

void ui_show_status(const char *status) {
    if (!statusbar) return;
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, status);
}

void ui_loop(void) {
    g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
}
