#ifndef COMMANDS_H
#define COMMANDS_H
#include "../include/types.h"

/* One entry in the command table */
typedef struct {
    const char *name;        /* what the user types     */
    const char *help;        /* shown by 'help'         */
    void (*fn)(const char *args);
} cmd_t;

extern cmd_t  cmd_table[];
extern int    cmd_count;

#endif
