#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "fb.h"
#include "keyboard.h"
#include "uac.h"

#define W 420
#define H 30

int kb_handler(char key);
bool login(const char *pword);
static void set_env(char *name, char *value);
static int charpos = 0;
static char pword[W/H];

void clear_rect(void) {
    gradient_t gradient;
    gradient.A.r = 0xaa;
    gradient.A.g = 0xaa;
    gradient.A.b = 0xaa;
    gradient.dir = NONE;
    draw_rect(gradient, (width()-W)/2, (height()-H)/2, W, H);
}

int main(int argc, char **argv) {
    usleep(1000000); //ugly hack :(

    fb_init();
    disable_echo();

    for(ssize_t i=0; i<W/H; i++) {
        pword[i] = 0;
    }

    draw_bitmap("/etc/ttydm/bg.bmp", 0, 0, 1);
    draw_bitmap(next_user(), (width()-150)/2, 250, 1);
    clear_rect();

    pthread_t keyboard = keythread_start(&kb_handler);
    pthread_join(keyboard, NULL);

    printf("execing on pid: %i\n", getpid());
    char *cmd = "/bin/bash";
    execl(user_shell(), user_shell(), "-c", cmd, NULL);
}

int kb_handler(char key) {
    rgb_t circle;
    circle.r = 0x11;
    circle.g = 0x11;
    circle.b = 0x11;

    rgb_t circle_gr;
    circle_gr.r = 0xaa;
    circle_gr.g = 0xaa;
    circle_gr.b = 0xaa;

    char *prof = user_has_prof();
    if (prof) {
        draw_bitmap(prof, (width()-150)/2, 250, 1);
    }

    if (key == 27) { // escape
        enable_echo();
        //return -1;
    } else if (key == '\t') {
        char *old = username();
        char *new_path = next_user();
        draw_bitmap(new_path, (width()-150)/2, 250, 1);
    } else if (key == 127) { // backspace 
        charpos--;
        if (charpos == -1) {
            charpos = 0;
        }
        pword[charpos] = 0;
        draw_circle(circle_gr, (width()-W+H)/2+(charpos*H), height()/2, (H-1)/2);
    } else if (key == 10) { // enter
        puts("attempting login");
        if (login(pword)) {
            enable_echo();
            return -1;
        } else {
            // flash red or something
        }
    } else if (charpos < W/H) {
        draw_circle(circle, (width()-W+H)/2+(charpos*H), height()/2, (H-1)/2);
        pword[charpos] = key;
        charpos++;
    }
    return key;
}

void init_env(void) {
    chown("/dev/tty1", get_login_uid(), (gid_t)(-1));
    chmod("/dev/tty1", 0600 );

    if (setgid(get_login_gid())) {
        perror("ERROR: could not set GID");
        exit(1);
    }

    set_groups();

    // set uid for this user
    if (setuid(get_login_uid())) {
        perror("ERROR, coult not set UID");
        exit(1);
    }

    if(chdir(home_dir())) {
        fprintf(stderr, "ERROR: cannot change directory to: %s\n", home_dir());
    }

    set_env("USER", username());
    set_env("LOGNAME", username());

    set_env("PWD", home_dir());
    set_env("HOME", home_dir());

    set_env("SHELL", user_shell());

    set_env("PATH", "/bin:/usr/bin:/usr/local/bin");

    set_env("XAUTHORITY", "/home/ted/.Xauthority");
}

#ifdef __NO_SYSTEMD_SHIT
bool login(const char *pword) {
    if (check_pw(pword)) {
        init_env();
        return true;
    }
    return false;
}
static void set_env(char *name, char *value) {
    setenv(name, value, 0);
}
#else
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#define err(name)                                   \
    do {                                            \
        fprintf(stderr, "%s: %s\n", name,           \
                pam_strerror(pam_handle, result));  \
        end(result);                                \
        return false;                               \
    } while (1);   

pam_handle_t *pam_handle;
static int end(int last_result) {
    int result = pam_end(pam_handle, last_result);
    pam_handle = 0;
    return result;
}

static void set_env(char *name, char *value) {
    setenv(name, value, 0);
    // The `+ 2` is for the '=' and the null byte
    size_t name_value_len = strlen(name) + strlen(value) + 2;
    char *name_value = malloc(name_value_len);
    snprintf(name_value, name_value_len,  "%s=%s", name, value);
    pam_putenv(pam_handle, name_value);
    free(name_value);
}


static int conv(int num_msg, const struct pam_message **msg,
        struct pam_response **resp, void *appdata_ptr) {
    int i;

    *resp = calloc(num_msg, sizeof(struct pam_response));
    if (*resp == NULL) {
        return PAM_BUF_ERR;
    }

    int result = PAM_SUCCESS;
    for (i = 0; i < num_msg; i++) {
        char *username, *password;
        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_ON:
                username = ((char **) appdata_ptr)[0];
                (*resp)[i].resp = strdup(username);
                break;
            case PAM_PROMPT_ECHO_OFF:
                password = ((char **) appdata_ptr)[1];
                (*resp)[i].resp = strdup(password);
                break;
            case PAM_ERROR_MSG:
                fprintf(stderr, "%s\n", msg[i]->msg);
                result = PAM_CONV_ERR;
                break;
            case PAM_TEXT_INFO:
                printf("%s\n", msg[i]->msg);
                break;
        }
        if (result != PAM_SUCCESS) {
            break;
        }
    }

    if (result != PAM_SUCCESS) {
        free(*resp);
        *resp = 0;
    }

    return result;
}

bool login(const char *password) {
    const char *data[2] = {username(), password};
    struct pam_conv pam_conv = {
        conv, data
    };

    int result = pam_start("ttydm", username(), &pam_conv, &pam_handle);
    if (result != PAM_SUCCESS) {
        err("pam_start");
    }

    result = pam_authenticate(pam_handle, 0);
    if (result != PAM_SUCCESS) {
        err("pam_authenticate");
    }

    result = pam_acct_mgmt(pam_handle, 0);
    if (result != PAM_SUCCESS) {
        err("pam_acct_mgmt");
    }

    result = pam_setcred(pam_handle, PAM_ESTABLISH_CRED);
    if (result != PAM_SUCCESS) {
        err("pam_setcred");
    }

    result = pam_open_session(pam_handle, 0);
    if (result != PAM_SUCCESS) {
        pam_setcred(pam_handle, PAM_DELETE_CRED);
        err("pam_open_session");
    }

    init_env();

    puts("login successful");
    return true;
}
#endif
