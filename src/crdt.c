#include "crdt.h"
#include <stdlib.h>
#include <string.h>

CRDT *crdt_new(void) {
    CRDT *r = malloc(sizeof(CRDT));
    r->head = NULL;
    return r;
}

void crdt_insert(CRDT *r, ID prev, ID id, const char *text) {
    Node *n = malloc(sizeof(Node));
    n->id = id;
    n->text = strdup(text);
    n->tombstone = false;
    n->next = NULL;
    n->prev = NULL;
    // TODO: insert logic (CRDT merge)
}

void crdt_delete(CRDT *r, ID id) {
    // TODO: find node by id and tombstone it
}

char *crdt_to_string(CRDT *r) {
    // TODO: build text snapshot
    return strdup("");
}

void crdt_merge(CRDT *local, const char *remote_json) {
    // TODO: parse JSON ops and apply
}
