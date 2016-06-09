#include <pthread.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>

#include "keyboard.h"

static struct termios *oldtc;

void disable_echo(void) {
	struct termios newtc;
    oldtc = malloc(sizeof(struct termios));
    tcgetattr(fileno(stdin), oldtc);
    newtc = *oldtc;
    newtc.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &newtc);
}

void enable_echo(void) {
    tcsetattr(fileno(stdin), TCSANOW, oldtc);
    free(oldtc);
}

struct thread_data {
    int (*handler)(char);
};

void *key_listen(void *data) {
    struct thread_data _data = *((struct thread_data *)data);
    int (*handler)(char) = _data.handler;
    free(data);
    int i = -1;
    do {
        i = getchar();
        if (i != -1) {
            i = handler((char)i);
        }
    } while(i != -1);
}

pthread_t keythread_start(int (*handler)(char)) {
    struct thread_data *data = malloc(sizeof(struct thread_data));
    data->handler = handler;
    pthread_t thread;
    pthread_create(&thread, NULL, key_listen, data);
    return thread;
}
