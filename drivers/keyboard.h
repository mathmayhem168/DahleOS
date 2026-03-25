#ifndef KEYBOARD_H
#define KEYBOARD_H
#include "../include/types.h"
void keyboard_install(void);
char keyboard_getchar(void);   /* blocks until a key is pressed */
int  keyboard_poll(void);      /* 1 if key is waiting, 0 otherwise */
#endif
