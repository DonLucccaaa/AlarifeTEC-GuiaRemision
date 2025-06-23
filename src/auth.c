#include "auth.h"
#include "db.h"
#include <string.h>

// Define tu código de administrador. En producción, podría leerse de config o generarse dinámico.
static const char *ADMIN_CODE = "ADMIN123";

bool check_admin_code(const char *code) {
    return (code && strcmp(code, ADMIN_CODE) == 0);
}

bool login_check(const char *user, const char *pass, int *user_id, int *role_id) {
    return db_verify_user(user, pass, user_id, role_id);
}

bool register_user(const char *user, const char *email, const char *pass, int role_code) {
    return db_create_user(user, email, pass, role_code);
}
