#pragma once
#include <stdint.h>

uint64_t utils_generate_site_id(void);
void utils_log(const char *fmt, ...);
char *utils_read_file(const char *path);
