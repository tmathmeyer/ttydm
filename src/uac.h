#ifndef uac_h
#define uac_h

#include <stdbool.h>

char *next_user(void);
char *username(void);
char *user_has_prof(void);
char *home_dir(void);
char *user_shell(void);
bool check_pw(char *);
uid_t get_login_uid(void);
gid_t get_login_gid(void);
void set_groups(void);

#endif
