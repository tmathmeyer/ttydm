#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#include "fb.h"
#include "keyboard.h"
#include "uac.h"

#define W 420
#define H 30

int kb_handler(char key);
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

    if (key == 27) { // escape
        enable_echo();
        return -1;
    } else if (key == '\t') {
        char *old = username();
        char *new_path = next_user();
        if (strcmp(old, username())) {
            draw_bitmap(new_path, (width()-150)/2, 250, 1);
        }
    } else if (key == 127) { // backspace 
        charpos--;
        pword[charpos] = 0;
        draw_circle(circle_gr, (width()-W+H)/2+(charpos*H), height()/2, (H-1)/2);
        if (charpos == -1) {
            charpos = 0;
        }
    } else if (key == 10) { // enter
        if (check_pw(pword)) {
            printf("user: %s is logged in!\n", username());
        }
    } else if (charpos < W/H) {
        draw_circle(circle, (width()-W+H)/2+(charpos*H), height()/2, (H-1)/2);
        pword[charpos] = key;
        charpos++;
    }
    return key;
}