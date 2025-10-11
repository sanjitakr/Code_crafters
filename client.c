#include <gtk/gtk.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <jansson.h>
#include <unistd.h>
#include <stdlib.h>


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define BUF_SIZE 4096


GtkWidget *text_view;
GtkWidget *status_label;
int sockfd;
int client_id = 0;


// --- Word count ---
static void update_word_count(GtkTextBuffer *buf, gpointer user_data) {
   GtkTextIter start, end;
   gtk_text_buffer_get_bounds(buf, &start, &end);
   gchar *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);


   int count = 0;
   gboolean in_word = FALSE;
   for (int i = 0; text[i]; i++) {
       if (g_ascii_isspace(text[i])) in_word = FALSE;
       else if (!in_word) { in_word = TRUE; count++; }
   }


   gchar *status = g_strdup_printf("Word Count: %d", count);
   gtk_label_set_text(GTK_LABEL(status_label), status);
   g_free(status);
   g_free(text);
}


// --- Formatting Tags ---
static void create_tags(GtkTextBuffer *buf) {
   gtk_text_buffer_create_tag(buf, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
   gtk_text_buffer_create_tag(buf, "italic", "style", PANGO_STYLE_ITALIC, NULL);
   gtk_text_buffer_create_tag(buf, "underline", "underline", PANGO_UNDERLINE_SINGLE, NULL);
   gtk_text_buffer_create_tag(buf, "red", "foreground", "red", NULL);
   gtk_text_buffer_create_tag(buf, "blue", "foreground", "blue", NULL);
   gtk_text_buffer_create_tag(buf, "green", "foreground", "green", NULL);
   gtk_text_buffer_create_tag(buf, "small", "scale", 0.8, NULL);
   gtk_text_buffer_create_tag(buf, "normal", "scale", 1.0, NULL);
   gtk_text_buffer_create_tag(buf, "big", "scale", 1.5, NULL);
}


// --- Toggle formatting ---
static void toggle_tag(GtkTextBuffer *buf, const char *tag_name) {
   GtkTextIter start, end;
   if (!gtk_text_buffer_get_selection_bounds(buf, &start, &end)) return;


   GtkTextTag *tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buf), tag_name);
   if (!tag) return;


   gboolean has_tag = TRUE;
   GtkTextIter iter = start;
   while (gtk_text_iter_compare(&iter, &end) < 0) {
       if (!gtk_text_iter_has_tag(&iter, tag)) { has_tag = FALSE; break; }
       gtk_text_iter_forward_char(&iter);
   }


   if (has_tag)
       gtk_text_buffer_remove_tag(buf, tag, &start, &end);
   else
       gtk_text_buffer_apply_tag(buf, tag, &start, &end);
}


// --- Button callback ---
static void on_format_clicked(GtkWidget *widget, gpointer tag_name) {
   GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
   if (!buf) return;  // safety check
   toggle_tag(buf, (const char *)tag_name);
}


// --- Network thread apply remote ---
typedef struct {
   gchar *text;
   int pos;
} RemoteInsert;


gboolean apply_remote_text(gpointer data) {
   RemoteInsert *ri = (RemoteInsert *)data;
   GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
   GtkTextIter iter;
   gtk_text_buffer_get_iter_at_offset(buf, &iter, ri->pos);
   gtk_text_buffer_insert(buf, &iter, ri->text, -1);
   g_free(ri->text);
   free(ri);
   return FALSE;
}


// --- Thread to receive messages ---
void *network_thread(void *arg) {
   char buf[BUF_SIZE];
   while (1) {
       int n = recv(sockfd, buf, BUF_SIZE - 1, 0);
       if (n <= 0) break;
       buf[n] = '\0';


       json_error_t error;
       json_t *msg = json_loads(buf, 0, &error);
       if (!msg) continue;


       int sender = json_integer_value(json_object_get(msg, "sender"));
       if (sender == client_id) { json_decref(msg); continue; }


       const char *type = json_string_value(json_object_get(msg, "type"));
       if (strcmp(type, "insert") == 0) {
           int pos = json_integer_value(json_object_get(msg, "pos"));
           const char *text = json_string_value(json_object_get(msg, "text"));
           RemoteInsert *ri = malloc(sizeof(RemoteInsert));
           ri->pos = pos;
           ri->text = g_strdup(text);
           g_idle_add(apply_remote_text, ri);
       }


       json_decref(msg);
   }
   return NULL;
}


// --- Insert text callback ---
static void on_insert_text(GtkTextBuffer *buf, GtkTextIter *location, gchar *text, gint len, gpointer user_data) {
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("insert"));
   json_object_set_new(msg, "pos", json_integer(gtk_text_iter_get_offset(location)));
   json_object_set_new(msg, "text", json_stringn(text, len));
   char *data = json_dumps(msg, 0);
   send(sockfd, data, strlen(data), 0);
   free(data);
   json_decref(msg);
}


// --- Activate GTK ---
static void activate(GtkApplication *app, gpointer user_data) {
   GtkWidget *window = gtk_application_window_new(app);
   gtk_window_set_title(GTK_WINDOW(window), "GTK4 Collaborative Editor");
   gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);


   GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
   gtk_window_set_child(GTK_WINDOW(window), vbox);


   // Toolbar
   GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
   gtk_box_append(GTK_BOX(vbox), toolbar);


   const gchar *buttons[] = {"Bold", "Italic", "Underline", "Red", "Blue", "Green", "Small", "Normal", "Big"};
   const gchar *tags[] = {"bold", "italic", "underline", "red", "blue", "green", "small", "normal", "big"};


   for (int i = 0; i < 9; i++) {
       GtkWidget *btn = gtk_button_new_with_label(buttons[i]);
       gtk_box_append(GTK_BOX(toolbar), btn);
       g_signal_connect(btn, "clicked", G_CALLBACK(on_format_clicked), (gpointer)tags[i]);
   }


   // Text area
   GtkWidget *scrolled = gtk_scrolled_window_new();
   gtk_widget_set_vexpand(scrolled, TRUE);
   gtk_box_append(GTK_BOX(vbox), scrolled);


   text_view = gtk_text_view_new();
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
   gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), text_view);


   GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
   create_tags(buf);
   g_signal_connect(buf, "changed", G_CALLBACK(update_word_count), NULL);
   g_signal_connect(buf, "insert-text", G_CALLBACK(on_insert_text), NULL);


   // Status bar
   status_label = gtk_label_new("Word Count: 0");
   gtk_widget_set_halign(status_label, GTK_ALIGN_END);
   gtk_box_append(GTK_BOX(vbox), status_label);


   gtk_window_present(GTK_WINDOW(window));
}


// --- Main ---
int main(int argc, char **argv) {
   // Connect to server
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   struct sockaddr_in serv_addr = {0};
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(SERVER_PORT);
   inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);
   if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
       perror("connect"); return 1;
   }


   // Launch network thread
   pthread_t tid;
   pthread_create(&tid, NULL, network_thread, NULL);


   GtkApplication *app = gtk_application_new("org.example.collab", G_APPLICATION_DEFAULT_FLAGS);
   g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
   int status = g_application_run(G_APPLICATION(app), argc, argv);
   g_object_unref(app);


   close(sockfd);
   return status;
}


