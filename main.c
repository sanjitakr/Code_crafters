#include <gtk/gtk.h>
#include <string.h>


GtkWidget *text_view;
GtkWidget *status_label;
GHashTable *toolbar_buttons; // key: tag name, value: GtkWidget* button


// --- Update Word Count ---
static void update_word_count(GtkTextBuffer *buffer, gpointer user_data) {
   GtkTextIter start, end;
   gtk_text_buffer_get_bounds(buffer, &start, &end);
   gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);


   int count = 0;
   gboolean in_word = FALSE;
   for (int i = 0; text[i] != '\0'; i++) {
       if (g_ascii_isspace(text[i])) in_word = FALSE;
       else if (!in_word) { in_word = TRUE; count++; }
   }


   gchar *status = g_strdup_printf("Word Count: %d", count);
   gtk_label_set_text(GTK_LABEL(status_label), status);
   g_free(status);
   g_free(text);
}


// --- Check if entire selection has a tag ---
static gboolean selection_has_tag(GtkTextBuffer *buffer, GtkTextTag *tag) {
   GtkTextIter start, end, iter;
   if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) return FALSE;


   iter = start;
   while (gtk_text_iter_compare(&iter, &end) < 0) {
       if (!gtk_text_iter_has_tag(&iter, tag)) return FALSE;
       gtk_text_iter_forward_char(&iter);
   }
   return TRUE;
}


// --- Toggle a tag with category exclusivity ---
static void toggle_tag(GtkTextBuffer *buffer, const gchar *tag_name, const gchar **exclusive_tags, int exclusive_count) {
   GtkTextIter start, end;
   if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) return;


   GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
   GtkTextTag *tag = gtk_text_tag_table_lookup(tag_table, tag_name);


   gboolean has_tag = selection_has_tag(buffer, tag);


   // Remove other tags in the same exclusive category
   for (int i = 0; i < exclusive_count; i++) {
       GtkTextTag *ex_tag = gtk_text_tag_table_lookup(tag_table, exclusive_tags[i]);
       if (ex_tag != tag)
           gtk_text_buffer_remove_tag(buffer, ex_tag, &start, &end);
   }


   // Toggle the tag
   if (!has_tag)
       gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
   else
       gtk_text_buffer_remove_tag(buffer, tag, &start, &end);
}


// --- Update toolbar highlighting based on selection ---
static void update_toolbar_highlight(GtkTextBuffer *buffer, gpointer user_data) {
   GtkTextIter start, end;
   gboolean has_selection = gtk_text_buffer_get_selection_bounds(buffer, &start, &end);


   GHashTableIter iter;
   gpointer key, value;
   g_hash_table_iter_init(&iter, toolbar_buttons);


   while (g_hash_table_iter_next(&iter, &key, &value)) {
       const gchar *tag_name = (const gchar *)key;
       GtkWidget *button = GTK_WIDGET(value);


       GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
       GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);


       gboolean active = FALSE;
       if (has_selection)
           active = selection_has_tag(buffer, tag);


       // Highlight active tags
       gtk_widget_set_opacity(button, active ? 0.6 : 1.0);
   }
}


// --- Handle format button clicks ---
static void on_format_button_clicked(GtkWidget *widget, gpointer tag_name) {
   GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));


   const gchar *color_tags[] = {"red", "blue", "green"};
   const gchar *size_tags[] = {"small", "normal", "big"};


   const gchar *tname = (const gchar *)tag_name;


   if (g_strcmp0(tname, "red") == 0 || g_strcmp0(tname, "blue") == 0 || g_strcmp0(tname, "green") == 0)
       toggle_tag(buffer, tname, color_tags, 3);
   else if (g_strcmp0(tname, "small") == 0 || g_strcmp0(tname, "normal") == 0 || g_strcmp0(tname, "big") == 0)
       toggle_tag(buffer, tname, size_tags, 3);
   else
       toggle_tag(buffer, tname, NULL, 0);  // bold, italic, underline


   update_toolbar_highlight(buffer, NULL);
}


// --- Create formatting tags ---
static void create_tags(GtkTextBuffer *buffer) {
   gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
   gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
   gtk_text_buffer_create_tag(buffer, "underline", "underline", PANGO_UNDERLINE_SINGLE, NULL);
   gtk_text_buffer_create_tag(buffer, "red", "foreground", "red", NULL);
   gtk_text_buffer_create_tag(buffer, "blue", "foreground", "blue", NULL);
   gtk_text_buffer_create_tag(buffer, "green", "foreground", "green", NULL);
   gtk_text_buffer_create_tag(buffer, "small", "scale", 0.8, NULL);
   gtk_text_buffer_create_tag(buffer, "normal", "scale", 1.0, NULL);
   gtk_text_buffer_create_tag(buffer, "big", "scale", 1.5, NULL);
}


// --- Keyboard shortcuts ---
static gboolean on_key_press(GtkEventControllerKey *controller,
                            guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
   gboolean ctrl = (state & GDK_CONTROL_MASK);
   if (!ctrl) return FALSE;


   switch (keyval) {
       case GDK_KEY_b: on_format_button_clicked(NULL, "bold"); break;
       case GDK_KEY_i: on_format_button_clicked(NULL, "italic"); break;
       case GDK_KEY_u: on_format_button_clicked(NULL, "underline"); break;
       case GDK_KEY_r: on_format_button_clicked(NULL, "red"); break;
       case GDK_KEY_l: on_format_button_clicked(NULL, "blue"); break;
       case GDK_KEY_g: on_format_button_clicked(NULL, "green"); break;
       case GDK_KEY_equal: on_format_button_clicked(NULL, "big"); break;
       case GDK_KEY_minus: on_format_button_clicked(NULL, "small"); break;
       default: return FALSE;
   }
   return TRUE;
}


// --- Activate callback ---
static void activate(GtkApplication *app, gpointer user_data) {
   GtkWidget *window = gtk_application_window_new(app);
   gtk_window_set_title(GTK_WINDOW(window), "GTK4 Rich Text Editor");
   gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);


   GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
   gtk_window_set_child(GTK_WINDOW(window), vbox);


   // Toolbar
   GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
   gtk_box_append(GTK_BOX(vbox), toolbar);


   const gchar *buttons[] = {"Bold", "Italic", "Underline", "Red", "Blue", "Green", "Small", "Normal", "Big"};
   const gchar *tags[] = {"bold", "italic", "underline", "red", "blue", "green", "small", "normal", "big"};


   toolbar_buttons = g_hash_table_new(g_str_hash, g_str_equal);


   for (int i = 0; i < 9; i++) {
       GtkWidget *btn = gtk_button_new_with_label(buttons[i]);
       gtk_box_append(GTK_BOX(toolbar), btn);
       g_signal_connect(btn, "clicked", G_CALLBACK(on_format_button_clicked), (gpointer)tags[i]);
       g_hash_table_insert(toolbar_buttons, (gpointer)tags[i], btn);
   }


   // Text area
   GtkWidget *scrolled = gtk_scrolled_window_new();
   gtk_widget_set_vexpand(scrolled, TRUE);
   gtk_box_append(GTK_BOX(vbox), scrolled);


   text_view = gtk_text_view_new();
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
   gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), text_view);


   // Status bar
   status_label = gtk_label_new("Word Count: 0");
   gtk_widget_set_halign(status_label, GTK_ALIGN_END);
   gtk_box_append(GTK_BOX(vbox), status_label);


   GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
   create_tags(buffer);
   g_signal_connect(buffer, "changed", G_CALLBACK(update_word_count), NULL);
   g_signal_connect(buffer, "mark-set", G_CALLBACK(update_toolbar_highlight), NULL);


   // Keyboard shortcut controller
   GtkEventController *controller = gtk_event_controller_key_new();
   g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_press), NULL);
   gtk_widget_add_controller(window, controller);


   gtk_window_present(GTK_WINDOW(window));
}


int main(int argc, char **argv) {
   GtkApplication *app = gtk_application_new("org.example.texteditor", G_APPLICATION_DEFAULT_FLAGS);
   g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
   int status = g_application_run(G_APPLICATION(app), argc, argv);
   g_object_unref(app);


   g_hash_table_destroy(toolbar_buttons);
   return status;
}
