#include "network.h"
#include "utils.h"

bool network_init(const char *host, int port) {
    utils_log("Network started on %s:%d", host, port);
    // TODO: Initialize WebSocket server/client
    return true;
}

void network_broadcast(const char *msg) {
    // TODO: send message to all peers
}

void network_poll(void) {
    // TODO: handle incoming messages
}

void network_shutdown(void) {
    utils_log("Network shutting down.");
}
