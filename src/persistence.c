#include "persistence.h"
#include "utils.h"

void persistence_init(void) {
    utils_log("Persistence initialized.");
}

void persistence_save_snapshot(const char *doc_id, CRDT *crdt) {
    // TODO: Save to SQLite
}

void persistence_load_snapshot(const char *doc_id, CRDT *crdt) {
    // TODO: Load from SQLite
}
