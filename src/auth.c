#include "auth.h"
#include "utils.h"

bool auth_verify_token(const char *token) {
    // TODO: validate with libjwt
    return true;
}

bool auth_can_edit(const char *user_id, const char *doc_id) {
    // TODO: lookup permissions
    return true;
}
