#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define MAX_CLIENTS 10

static struct lws *clients[MAX_CLIENTS];
static int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Simple username/password store
typedef struct {
    const char* username;
    const char* password;
} User;

User authorized_users[] = {
    {"alice", "1234"},
    {"bob", "abcd"},
    {NULL, NULL} // sentinel
};

// --- Broadcast to all clients ---
void broadcast_message(const char *msg, size_t len) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i]) lws_write(clients[i], (unsigned char *)msg, len, LWS_WRITE_TEXT);
    }
    pthread_mutex_unlock(&clients_mutex);
}

// --- Check authorization ---
int check_auth(const char* username, const char* password) {
    for (int i = 0; authorized_users[i].username; i++) {
        if (strcmp(username, authorized_users[i].username) == 0 &&
            strcmp(password, authorized_users[i].password) == 0) {
            return 1;
        }
    }
    return 0;
}

// --- WebSocket callback ---
static int callback_editor(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            pthread_mutex_lock(&clients_mutex);
            if (client_count < MAX_CLIENTS) clients[client_count++] = wsi;
            pthread_mutex_unlock(&clients_mutex);
            printf("Client connected. Total clients: %d\n", client_count);
            break;

        case LWS_CALLBACK_RECEIVE:
        {
            // Expect JSON like: {"type":"auth","username":"alice","password":"1234"}
            char* msg = (char*)in;
            if (strncmp(msg, "{\"type\":\"auth\"", 14) == 0) {
                char username[64], password[64];
                sscanf(msg, "{\"type\":\"auth\",\"username\":\"%63[^\"]\",\"password\":\"%63[^\"]\"}", username, password);
                char resp[128];
                if (check_auth(username, password)) {
                    snprintf(resp, sizeof(resp), "{\"type\":\"auth_resp\",\"status\":\"ok\"}");
                    lws_write(wsi, (unsigned char*)resp, strlen(resp), LWS_WRITE_TEXT);
                    printf("User %s authorized\n", username);
                } else {
                    snprintf(resp, sizeof(resp), "{\"type\":\"auth_resp\",\"status\":\"fail\"}");
                    lws_write(wsi, (unsigned char*)resp, strlen(resp), LWS_WRITE_TEXT);
                    printf("User %s failed authorization\n", username);
                }
            } else {
                // Broadcast all other updates
                broadcast_message((const char *)in, len);
            }
            break;
        }

        case LWS_CALLBACK_CLOSED:
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < client_count; i++) {
                if (clients[i] == wsi) {
                    for (int j = i; j < client_count - 1; j++) clients[j] = clients[j + 1];
                    clients[client_count - 1] = NULL;
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            printf("Client disconnected. Total clients: %d\n", client_count);
            break;

        default:
            break;
    }
    return 0;
}

// --- Protocols ---
static struct lws_protocols protocols[] = {
    { "editor-protocol", callback_editor, 0, 4096 },
    { NULL, NULL, 0, 0 }
};

int main() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = 9000;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "libwebsockets init failed\n");
        return -1;
    }

    printf("Server running on ws://localhost:9000\n");

    while (1) lws_service(context, 100);

    lws_context_destroy(context);
    return 0;
}


