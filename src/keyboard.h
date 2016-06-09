#ifndef keyboard_h
#define keyboard_h

#include <pthread.h>

void disable_echo(void);
void enable_echo(void);
pthread_t keythread_start(int (handler)(char));


#endif
