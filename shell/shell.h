#ifndef SHELL_H
#define SHELL_H

/* ================================================================
   shell/shell.h  –  Customise your shell here
   ================================================================ */

#define OS_NAME       "DahleOS"
#define OS_VERSION    "1.0.0"

/* The prompt printed before every line of input */
#define SHELL_PROMPT  "DahleOS > "

/* Maximum characters per input line */
#define LINE_MAX      256

/* Number of commands kept in history (scroll with arrow keys) */
#define HISTORY_MAX   20

/* ================================================================ */
void shell_run(void);
#endif
