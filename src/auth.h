#ifndef AUTH_H
#define AUTH_H
#define ROLE_ADMIN 1
#define ROLE_ENCARGADO 2
#define ROLE_CONSULTOR 3

#include <stdbool.h>

bool login_check(const char *user, const char *pass, int *user_id, int *role_id);
bool register_user(const char *user, const char *email, const char *pass, int role_code);
bool check_admin_code(const char *code);

#endif 
