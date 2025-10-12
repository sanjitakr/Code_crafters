#include <gtk/gtk.h>
#include <jansson.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ui.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000

int sock;
GtkTextBuffer *buffer;

// --- Insert callback ---
void on_insert(GtkTextBuffer *buf, GtkTextIter *location, gchar *text, gint len, gpointer user_data) {
    json_t *obj = json_object();
    json_object_set_new(obj, "type", json_string("insert"));
    int offset = gtk_text_iter_get_offset(location);
    json_object_set_new(obj, "offset", json_integer(offset));
    json_object_set_new(obj, "text", json_string(text));

    char *msg = json_dumps(obj, 0);
    send(sock, msg, strlen(msg), 0);
    free(msg);
    json_decref(obj);
}

// --- Delete callback ---
void on_delete(GtkTextBuffer *buf, GtkTextIter *start, GtkTextIter *end, gpointer user_data) {
    int s_offset = gtk_text_iter_get_offset(start);
    int e_offset = gtk_text_iter_get_offset(end);

    json_t *obj = json_object();
    json_object_set_new(obj, "type", json_string("delete"));
    json_object_set_new(obj, "start", json_integer(s_offset));
    json_object_set_new(obj, "end", json_integer(e_offset));

    char *msg = json_dumps(obj, 0);
    send(sock, msg, strlen(msg), 0);
    free(msg);
    json_decref(obj);
}

// --- Apply remote edit in GTK thread ---
gboolean apply_remote_edit(gpointer data) {
    json_t *obj = (json_t *)data;
    const char *type = json_string_value(json_object_get(obj, "type"));
    if (strcmp(type, "insert") == 0) {
        int offset = json_integer_value(json_object_get(obj, "offset"));
        const char *text = json_string_value(json_object_get(obj, "text"));
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset(buffer, &iter, offset);
        gtk_text_buffer_insert(buffer, &iter, text, -1);
    } else if (strcmp(type, "delete") == 0) {
        int start = json_integer_value(json_object_get(obj, "start"));
        int end = json_integer_value(json_object_get(obj, "end"));
        GtkTextIter s_iter, e_iter;
        gtk_text_buffer_get_iter_at_offset(buffer, &s_iter, start);
        gtk_text_buffer_get_iter_at_offset(buffer, &e_iter, end);
        gtk_text_buffer_delete(buffer, &s_iter, &e_iter);
    }
    json_decref(obj);
    return FALSE;
}

// --- Receive thread ---
void *recv_thread(void *arg) {
    char buf[4096];
    int len;
    while ((len = recv(sock, buf, sizeof(buf)-1, 0)) > 0) {
        buf[len] = '\0';
        json_error_t error;
        json_t *obj = json_loads(buf, 0, &error);
        if (obj) g_idle_add(apply_remote_edit, obj);
    }
    return NULL;
}

// --- Setup callbacks after UI ready ---
void setup_callbacks(void) {
    buffer = ui_get_text_buffer();
    g_signal_connect(buffer, "insert-text", G_CALLBACK(on_insert), NULL);
    g_signal_connect(buffer, "delete-range", G_CALLBACK(on_delete), NULL);

    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, NULL);
}

int main(int argc, char **argv) {
    // --- Connect to server ---
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    // --- Initialize GTK UI ---
    ui_init(argc, argv);
    ui_loop(setup_callbacks);

    close(sock);
    return 0;
}

