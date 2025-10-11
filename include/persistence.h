#pragma once
#include "crdt.h"

void persistence_init(void);
void persistence_save_snapshot(const char *doc_id, CRDT *crdt);
void persistence_load_snapshot(const char *doc_id, CRDT *crdt);
