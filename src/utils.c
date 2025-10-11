#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

uint64_t utils_generate_site_id(void) {
    srand(time(NULL));
    return ((uint64_t)rand() << 32) | rand();
}

void utils_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[LOG] ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

char *utils_read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}
