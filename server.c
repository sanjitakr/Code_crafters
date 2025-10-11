// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <jansson.h>


#define PORT 12345
#define MAX_CLIENTS 10
#define BUF_SIZE 4096


typedef struct {
   int sockfd;
   int id;
} client_t;


client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;


// Add a client
void add_client(client_t *cl) {
   pthread_mutex_lock(&clients_mutex);
   if (client_count < MAX_CLIENTS) {
       clients[client_count++] = cl;
   }
   pthread_mutex_unlock(&clients_mutex);
}


// Remove a client
void remove_client(int id) {
   pthread_mutex_lock(&clients_mutex);
   for (int i = 0; i < client_count; i++) {
       if (clients[i]->id == id) {
           close(clients[i]->sockfd);
           free(clients[i]);
           for (int j = i; j < client_count - 1; j++)
               clients[j] = clients[j + 1];
           client_count--;
           break;
       }
   }
   pthread_mutex_unlock(&clients_mutex);
}


// Broadcast message to all clients except sender
void broadcast(json_t *msg, int sender_id) {
   char *data = json_dumps(msg, 0);
   pthread_mutex_lock(&clients_mutex);
   for (int i = 0; i < client_count; i++) {
       if (clients[i]->id != sender_id) {
           send(clients[i]->sockfd, data, strlen(data), 0);
       }
   }
   pthread_mutex_unlock(&clients_mutex);
   free(data);
}


// Handle a single client
void *handle_client(void *arg) {
   client_t *cl = (client_t *)arg;
   char buf[BUF_SIZE];


   while (1) {
       int n = recv(cl->sockfd, buf, BUF_SIZE - 1, 0);
       if (n <= 0) break;
       buf[n] = '\0';


       json_error_t error;
       json_t *msg = json_loads(buf, 0, &error);
       if (!msg) continue;


       // Attach sender ID to the message
       json_object_set_new(msg, "sender", json_integer(cl->id));


       broadcast(msg, cl->id);
       json_decref(msg);
   }


   printf("Client %d disconnected\n", cl->id);
   remove_client(cl->id);
   pthread_detach(pthread_self());
   return NULL;
}


int main() {
   int listenfd = socket(AF_INET, SOCK_STREAM, 0);
   if (listenfd < 0) { perror("socket"); exit(1); }


   struct sockaddr_in serv_addr = {0};
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(PORT);


   if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
       perror("bind"); exit(1);
   }


   if (listen(listenfd, MAX_CLIENTS) < 0) {
       perror("listen"); exit(1);
   }


   printf("Server started on port %d\n", PORT);


   int next_id = 1;
   while (1) {
       struct sockaddr_in cli_addr;
       socklen_t cli_len = sizeof(cli_addr);
       int connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
       if (connfd < 0) continue;


       client_t *cl = malloc(sizeof(client_t));
       cl->sockfd = connfd;
       cl->id = next_id++;
       add_client(cl);


       printf("New client connected: ID %d\n", cl->id);


       pthread_t tid;
       pthread_create(&tid, NULL, handle_client, (void *)cl);
       pthread_detach(tid);
   }


   close(listenfd);
   return 0;
}


