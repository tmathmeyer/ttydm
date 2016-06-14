#include <errno.h>
#include <crypt.h>
#include <shadow.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <grp.h>

#include "uac.h"

static struct passwd *passwd = NULL;
static int runs = 0;

char *user_has_prof();
char *next_user(void) {
    char *logo = NULL;
    do {
        passwd = getpwent();
        if (passwd == NULL) { // done with users!
            if (runs == 0) { // there are no users with a profile
                exit(1);
            }
            setpwent(); // rewind
        }
    } while(!(logo = user_has_prof()));
    runs = 1;
    return logo;
}

char *user_has_prof() {
    char *uname = username();
    if (!uname) {
        return NULL;
    }
    char *new = calloc(strlen(uname)+5+strlen("/etc/ttydm/"), sizeof(char)); // .bmp
    if (new) {
        strcat(new, "/etc/ttydm/");
        strcat(new, uname);
        strcat(new, ".bmp");
        if (access(new, F_OK) != -1) {
            return new;
        }
        free(new);
    }
    return NULL;
}

uid_t get_login_uid() {
    if (!passwd) {
        exit(1); // this cant ever happen anyway
    }
    return passwd->pw_uid;
}

gid_t get_login_gid() {
    if (!passwd) {
        exit(1); // this cant ever happen anyway
    }
    return passwd->pw_gid;
}

char *username() {
    if (passwd != NULL) {
        return passwd->pw_name;
    }
}

char *user_shell() {
    if (passwd != NULL) {
        if (passwd->pw_shell) {
            return passwd->pw_shell;
        } else {
            return "/bin/bash";
        }
    }
}

char *home_dir() {
    if (passwd != NULL) {
        return passwd->pw_dir;
    }
}

char *getpwd(void) {
    char *rval = passwd->pw_passwd;
    if(rval[0] == 'x' && rval[1] == '\0'){
        struct spwd *sp;
        sp = getspnam(username());
        if(!sp){
            perror("cannot verify passwords");
            return NULL;
        }
        rval = sp->sp_pwdp;
    }
    return rval;
}

bool check_pw(char *inpass) {
    if (passwd != NULL) {
        char *pass = getpwd();
        if (pass) {
            char *cr = crypt(inpass, pass);
            if (cr) {
                return !strcmp(cr, pass);
            }
        }
    }
    return false;
}

void set_groups() {
    if (passwd == NULL) {
        exit(1);
    }

    ssize_t gid_ct = 8;
    ssize_t gid_rl = 0;
    gid_t *gids = calloc(sizeof(gid_t), gid_ct);

    char *uname = username();
    size_t ulen = strlen(uname);
    struct group *grp;
    while(grp = getgrent()) {
        char **members = grp->gr_mem;
        while(*members) {
            size_t tlen = strlen(*members);
            if ((tlen==ulen && strncmp(*members, uname, tlen)==0) || strncmp("input", grp->gr_name, 6)) {
                gids[gid_rl] = grp->gr_gid;
                gid_rl++;
                if (gid_rl == gid_ct) {
                    gid_ct*=2;
                    gids = realloc(gids, sizeof(gid_t)*gid_ct);
                }
            }
            members++;
        }
    }

    if (setgroups(gid_rl, gids)) {
        perror("ERROR: could not set groups");
        exit(1);
    }

    free(gids);

    endgrent();
}
