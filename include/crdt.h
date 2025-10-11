#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t site;
    uint64_t seq;
} ID;

typedef struct Node {
    ID id;
    char *text;
    bool tombstone;
    struct Node *next;
    struct Node *prev;
} Node;

typedef struct {
    Node *head;
} CRDT;

CRDT *crdt_new(void);
void crdt_free(CRDT *r);
void crdt_insert(CRDT *r, ID prev, ID id, const char *text);
void crdt_delete(CRDT *r, ID id);
char *crdt_to_string(CRDT *r);
void crdt_merge(CRDT *local, const char *remote_json);
