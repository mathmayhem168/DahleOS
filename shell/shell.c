/* ================================================================
   shell/shell.c  –  Interactive shell loop
   ================================================================
   You can customise:
     • The welcome message  (search "Welcome to")
     • The prompt colour    (search "SHELL_PROMPT")
     • Key handling below the "Main loop" comment
   ================================================================ */

#include "shell.h"
#include "commands.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../libc/string.h"
#include "../libc/mem.h"

/* ---- input line state ---- */
static char line[LINE_MAX];
static int  len = 0;

/* ---- history ---- */
static char hist[HISTORY_MAX][LINE_MAX];
static int  hist_n = 0;   /* total entries ever added (wraps)  */
static int  hist_i = -1;  /* current scroll position (-1=live) */
static char live[LINE_MAX]; /* snapshot of live input before nav */

static void hist_push(const char *s) {
    if (!*s) return;
    if (hist_n && strcmp(hist[(hist_n - 1) % HISTORY_MAX], s) == 0) return;
    strncpy(hist[hist_n % HISTORY_MAX], s, LINE_MAX - 1);
    hist_n++;
}

/* Return entry at scroll offset (1=newest, 2=one before …). NULL if OOB. */
static const char *hist_get(int offset) {
    int idx = hist_n - offset;
    if (idx < 0 || offset > hist_n) return NULL;
    return hist[idx % HISTORY_MAX];
}

/* Erase current visible line, replace with s, update state. */
static void redraw_line(const char *s) {
    while (len-- > 0) screen_backspace();
    strncpy(line, s, LINE_MAX - 1);
    line[LINE_MAX - 1] = '\0';
    len = (int)strlen(line);
    kprint(line);
}

/* ---- command dispatch ---- */
static void run(const char *input) {
    while (*input == ' ') input++;
    if (!*input) return;

    char name[64] = {0};
    int i = 0;
    while (input[i] && input[i] != ' ' && i < 63) { name[i] = input[i]; i++; }
    const char *args = input + i;
    while (*args == ' ') args++;

    for (int c = 0; c < cmd_count; c++) {
        if (strcmp(cmd_table[c].name, name) == 0) {
            cmd_table[c].fn(args);
            return;
        }
    }
    kprint_color("Unknown command: ", LRED, BLACK);
    kprint(name);
    kprint("  (type 'help')\n");
}

static void print_prompt(void) {
    kprint_color(SHELL_PROMPT, LGREEN, BLACK);
}

/* ================================================================
   Main loop
   Reads keystrokes and dispatches to run() on Enter.
   ================================================================ */
void shell_run(void) {
    kprint_color("  Welcome to " OS_NAME " " OS_VERSION "!\n", LCYAN, BLACK);
    kprint("  Type ");
    kprint_color("help", LGREEN, BLACK);
    kprint(" to see available commands.\n\n");

    print_prompt();
    memset(line, 0, LINE_MAX);
    memset(live, 0, LINE_MAX);
    len = 0;
    hist_i = -1;

    while (1) {
        char c = keyboard_getchar();

        /* ── Enter ── */
        if (c == '\n') {
            kprint("\n");
            line[len] = '\0';
            hist_push(line);
            hist_i = -1;
            run(line);
            memset(line, 0, LINE_MAX);
            len = 0;
            print_prompt();
            continue;
        }

        /* ── Backspace ── */
        if (c == '\b') {
            if (len > 0) { line[--len] = '\0'; screen_backspace(); }
            continue;
        }

        /* ── Tab: complete first matching command name ── */
        if (c == '\t') {
            for (int i = 0; i < cmd_count; i++) {
                if (strncmp(cmd_table[i].name, line, (size_t)len) == 0) {
                    const char *rest = cmd_table[i].name + len;
                    while (*rest && len < LINE_MAX - 1) {
                        line[len++] = *rest;
                        kprint_char(*rest++);
                    }
                    break;
                }
            }
            continue;
        }

        /* ── Ctrl-P (0x10): scroll history up (older) ── */
        if (c == 0x10) {
            int next = hist_i + 1;
            const char *entry = hist_get(next);
            if (entry) {
                if (hist_i == -1) strncpy(live, line, LINE_MAX - 1);
                hist_i = next;
                redraw_line(entry);
            }
            continue;
        }

        /* ── Ctrl-N (0x0E): scroll history down (newer) ── */
        if (c == 0x0E) {
            if (hist_i > 1) {
                hist_i--;
                const char *entry = hist_get(hist_i);
                if (entry) redraw_line(entry);
            } else if (hist_i == 1) {
                hist_i = -1;
                redraw_line(live);
            }
            continue;
        }

        /* ── Printable character ── */
        if ((unsigned char)c >= 32 && (unsigned char)c < 127) {
            if (len < LINE_MAX - 1) {
                line[len++] = c;
                kprint_char(c);
            }
            continue;
        }
    }
}

// shell.c
void shell_execute(const char *input) {
    // parse command name and args from input
    char cmd[32] = {0};
    int i = 0;

    while (input[i] && input[i] != ' ' && i < 31)
        cmd[i] = input[i++];
    while (input[i] == ' ') i++;
    const char *args = &input[i];

    // your existing command dispatch
    if (kstrcmp(cmd, "echo") == 0)       cmd_echo(args);
    else if (kstrcmp(cmd, "clear") == 0) cmd_clear(args);
    else if (kstrcmp(cmd, "repeat") == 0) cmd_repeat(args);
    // ... rest of your commands
    else {
        kprint("Unknown command: ");
        kprint(cmd);
        kprint("\n");
    }
}