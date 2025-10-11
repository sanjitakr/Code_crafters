#pragma once
#include <stdbool.h>

bool auth_verify_token(const char *token);
bool auth_can_edit(const char *user_id, const char *doc_id);
