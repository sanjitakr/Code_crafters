#include "sync.h"
#include "utils.h"

void sync_handle_remote_op(const char *json_op) {
    utils_log("Received remote op: %s", json_op);
}

void sync_push_local_ops(void) {
    // TODO: send queued ops
}

void sync_handle_conflicts(CRDT *crdt) {
    // TODO: detect overlapping edits
}
