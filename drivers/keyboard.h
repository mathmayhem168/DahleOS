#ifndef KEYBOARD_H
#define KEYBOARD_H
#include "../include/types.h"
void keyboard_install(void);
char keyboard_getchar(void);        /* blocks until a key is pressed     */
int  keyboard_poll(void);           /* 1 if key is waiting, 0 otherwise  */
int  keyboard_shift_held(void);     /* 1 if a Shift key is held right now */

/* Special values returned by keyboard_getchar() for extended keys */
#define KEY_UP    '\x11'
#define KEY_DOWN  '\x12'
#define KEY_LEFT  '\x13'
#define KEY_RIGHT '\x14'
#endif
