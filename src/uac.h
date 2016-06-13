#ifndef uac_h
#define uac_h

#include <stdbool.h>

char *next_user(void);
char *username();
bool check_pw(char *);
uid_t getuid(void);
char *user_has_prof();
#endif
