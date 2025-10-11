#pragma once
#include "crdt.h"

void sync_handle_remote_op(const char *json_op);
void sync_push_local_ops(void);
void sync_handle_conflicts(CRDT *crdt);
