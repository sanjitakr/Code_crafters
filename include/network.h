#pragma once
#include <stdbool.h>

bool network_init(const char *host, int port);
void network_broadcast(const char *msg);
void network_poll(void);
void network_shutdown(void);
