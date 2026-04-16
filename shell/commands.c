/* ================================================================
   shell/commands.c  –  Add your own commands here!
   ================================================================

   HOW TO ADD A COMMAND
   --------------------
   1.  Write a function:   void cmd_yourname(const char *args) { ... }
   2.  Add one line to cmd_table[]:
           { "yourname", "short description", cmd_yourname },

   That's it.  The shell finds it automatically.
   ================================================================ */



#include "commands.h"
#include "shell.h"
#include "../drivers/screen.h"
#include "../drivers/gui.h"
#include "../drivers/port.h"
#include "../drivers/keyboard.h"
#include "../cpu/timer.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../fs/fs.h"


/* ----------------------------------------------------------------
   Forward declarations
   ---------------------------------------------------------------- */
static void cmd_help   (const char *args);
static void cmd_clear  (const char *args);
static void cmd_echo   (const char *args);
static void cmd_color  (const char *args);
static void cmd_uptime (const char *args);
static void cmd_info   (const char *args);
static void cmd_reboot (const char *args);
static void cmd_halt   (const char *args);
static void cmd_scp    (const char *args);
static void cmd_dahle  (const char *args);
static void cmd_kernelpanic(const char *args);
static void cmd_repeat(const char *args);
static void cmd_about(const char *args);
/* filesystem commands */
static void cmd_pwd   (const char *args);
static void cmd_ls    (const char *args);
static void cmd_mkdir (const char *args);
static void cmd_touch (const char *args);
static void cmd_rm    (const char *args);
static void cmd_cd    (const char *args);
static void cmd_cat   (const char *args);
/* computing commands */
static void cmd_alu  (const char *args);
/* modifying commands */
static void cmd_upper (const char *args);
static void cmd_lower (const char *args);
static void cmd_alias (const char *args);   // Maybe will be replaced with something like /.dahlerc

/* ================================================================
   DEFINITIONS  –  definitions for things like aliases
   ================================================================ */
#define ALIAS_MAX 16        // maximum amount of aliases
#define ALIAS_NAME_MAX 32   // maximum length for alias name
#define ALIAS_VAL_MAX 128   // maximum length for the alias's value or what it expands to






/* ================================================================
   STRUCTURES - use for helpers or commands
   ================================================================ */

typedef struct {
    char name[ALIAS_NAME_MAX];
    char value[ALIAS_VAL_MAX];
} alias_t;


/* ================================================================
   DATA - use for helpers only like an alias list
   ================================================================ */

static alias_t alias_table[ALIAS_MAX];
static int alias_count = 0;





/* ================================================================
   COMMAND TABLE  –  add your commands here
   ================================================================ */
cmd_t cmd_table[] = {
    { "help",   "List all commands",                   cmd_help   },
    { "clear",  "Clear the screen",                    cmd_clear  },
    { "echo",   "Print text  -  echo <message>",       cmd_echo   },
    { "color",  "Set colours  -  color <fg> <bg>",     cmd_color  },
    { "uptime", "Show how long the OS has been running", cmd_uptime },
    { "info",   "System information",                  cmd_info   },
    { "reboot", "Reboot the machine",                  cmd_reboot },
    { "halt",   "Shut down (halt CPU)",                cmd_halt   },
    /* ↓ Your new commands go here ↓ */
    { "scp",        "Print a random integer within a range", cmd_scp        },
    { "dahle",      "Launch the graphical desktop",          cmd_dahle      },
    { "kernelpanic","Simulate a kernel panic",               cmd_kernelpanic},
    { "repeat", "Repeat a command N times", cmd_repeat },
    { "about", "About this operating system", cmd_about },
    /* filesystem */
    { "pwd",   "Print working directory",         cmd_pwd   },
    { "ls",    "List directory contents",         cmd_ls    },
    { "mkdir", "Create a directory  -  mkdir <name>",  cmd_mkdir },
    { "touch", "Create an empty file  -  touch <name>", cmd_touch },
    { "rm",    "Remove a file  -  rm <name>",     cmd_rm    },
    { "cd",    "Change directory  -  cd <path>",  cmd_cd    },
    { "cat",   "Print file contents  -  cat <name>", cmd_cat },
    /* computing */
    { "alu",   "ALU op  -  alu <a> <b> <+|-|*|/>",  cmd_alu  },
    { "upper", "Outputs the input in all uppercases", cmd_upper },
    { "lower", "Outputs the input in all lowercases", cmd_lower },
    { "alias", "Aliases a special name for a built-in command", cmd_alias },
};

int cmd_count = (int)(sizeof(cmd_table) / sizeof(cmd_t));

/* ================================================================
   BUILT-IN IMPLEMENTATIONS
   ================================================================ */

static void cmd_help(const char *args) {
    (void)args;
    kprint("\n");
    kprint_color("  Commands\n", LCYAN, BLACK);
    kprint_color("  --------\n", LCYAN, BLACK);
    for (int i = 0; i < cmd_count; i++) {
        kprint("  ");
        kprint_color(cmd_table[i].name, LGREEN, BLACK);
        /* pad */
        int pad = 12 - (int)strlen(cmd_table[i].name);
        while (pad-- > 0) kprint_char(' ');
        kprint(cmd_table[i].help);
        kprint("\n");
    }
    kprint("\n");
}

static void cmd_clear(const char *args) { (void)args; screen_clear(); }

static void cmd_echo(const char *args) {
    if (!args || !*args) { kprint("\n"); return; }
    kprint(args);
    kprint("\n");
}

/* ---- color command ---- */
static const struct { const char *name; uint32_t color; } ctab[] = {
    {"black",   BLACK},    {"blue",     BLUE},
    {"green",   GREEN},    {"cyan",     CYAN},
    {"red",     RED},      {"magenta",  MAGENTA},
    {"brown",   BROWN},    {"lgrey",    LGREY},
    {"dgrey",   DGREY},    {"lblue",    LBLUE},
    {"lgreen",  LGREEN},   {"lcyan",    LCYAN},
    {"lred",    LRED},     {"lmagenta", LMAGENTA},
    {"lbrown",  LBROWN},   {"white",    WHITE},
};
#define CTAB_SIZE 16
/* Returns 0xFFFFFFFF if name not found (no valid palette entry uses that value) */
static uint32_t find_color(const char *s) {
    for (int i = 0; i < CTAB_SIZE; i++)
        if (strcmp(s, ctab[i].name) == 0) return ctab[i].color;
    return 0xFFFFFFFFu;
}

static void cmd_color(const char *args) {
    if (!args || !*args) {
        kprint("Usage: color <fg> <bg>\n");
        kprint("Names: black blue green cyan red magenta brown lgrey\n"
               "       dgrey lblue lgreen lcyan lred lmagenta lbrown white\n");
        return;
    }
    char a[16]={0}, b[16]={0};
    int i=0, j=0;
    while (args[i] && args[i]!=' ' && i<15) { a[i]=args[i]; i++; }
    while (args[i]==' ') i++;
    while (args[i] && args[i]!=' ' && j<15) { b[j++]=args[i++]; }
    uint32_t fc=find_color(a), bc=find_color(b);
    if (fc==0xFFFFFFFFu || bc==0xFFFFFFFFu) {
        kprint_color("Unknown color name.\n", LRED, BLACK); return;
    }
    screen_set_color(fc, bc);
    kprint("Color set.\n");
}

static void cmd_uptime(const char *args) {
    (void)args;
    uint32_t t = timer_ticks();
    uint32_t s = t/100, m = s/60, h = m/60;
    kprint("Uptime: ");
    kprint_int((int)h);   kprint("h ");
    kprint_int((int)(m%60)); kprint("m ");
    kprint_int((int)(s%60)); kprint("s  (");
    kprint_int((int)t);   kprint(" ticks @ 100 Hz)\n");
}

static void cmd_info(const char *args) {
    (void)args;
    kprint("\n");
    kprint_color("  " OS_NAME " " OS_VERSION "\n", LCYAN, BLACK);
    kprint("  Architecture  : x86 32-bit Protected Mode\n");
    kprint("  Display       : 800x600x32 VESA framebuffer\n");
    kprint("  Timer         : PIT 100 Hz\n");
    kprint("  Uptime        : "); kprint_int((int)(timer_ticks()/100)); kprint(" seconds\n");
    kprint("\n");
}

static void cmd_reboot(const char *args) {
    (void)args;
    kprint("Rebooting...\n");
    __asm__ volatile("cli");
    port_byte_out(0x64, 0xFE);
    __asm__ volatile("hlt");
}

static void cmd_halt(const char *args) {
    (void)args;
    kprint_color("System halted.\n", LRED, BLACK);
    __asm__ volatile("cli; hlt");
}

static int str_to_int(const char *s) {
    int n = 0, neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return neg ? -n : n;
}

static int rand_range(int range) {
    static uint32_t seed = 0;
    if (!seed) seed = timer_ticks() + 1;   /* lazy seed on first call */
    seed = seed * 1664525u + 1013904223u;  /* classic Knuth LCG       */
    /* mask to positive 31-bit value, then clamp to range */
    return (int)((seed >> 1) % (uint32_t)range);
}


static void cmd_scp(const char *args) {
    if (!args || !*args) {
        kprint("Usage: scp <a> <b>\n");
        kprint("Prints a random integer between a and b (inclusive).\n");
        return;
    }

    char a[12] = {0}, b[12] = {0};
    int i = 0, j = 0;
    while (args[i] && args[i] != ' ' && i < 11) { a[i] = args[i]; i++; }
    while (args[i] == ' ') i++;
    while (args[i] && args[i] != ' ' && j < 11) { b[j++] = args[i++]; }

    if (!a[0] || !b[0]) {
        kprint("Usage: scp <a> <b>\n");
        return;
    }

    int lo = str_to_int(a);
    int hi = str_to_int(b);

    if (lo > hi) {
        /* swap so order doesn't matter */
        int tmp = lo; lo = hi; hi = tmp;
    }

    int result = lo + rand_range(hi - lo + 1);

    kprint("Random [");
    kprint_int(lo); kprint(", "); kprint_int(hi);
    kprint("] -> ");
    kprint_color("SCP ", LCYAN, BLACK);
    kprint_color_int(result, LGREEN, BLACK);   /* see note below */
    kprint("\n");
}

/* =============================================================
   GUI — Desktop helpers and the `dahle` command
   =============================================================
   Three-window desktop with keyboard-driven window management.

   Controls:
     Tab         → cycle window focus
     Arrow keys  → move focused window
     ESC         → exit to shell
     Shift+ESC   → open App Launcher

   Window content coordinate helper
   ---------------------------------
   To place text or widgets inside a window drawn at (wx, wy):
     content_x = wx + GUI_PAD
     content_y = wy + GUI_TITLE_H + 2 + line * (screen_char_h() + LINE_GAP)
   Use dahle_wtext() / dahle_wkv() instead of computing this manually.
   ============================================================= */

#define LINE_GAP      4u

/* ── Desktop layout (three windows, centred on 800-px screen) ── */
#define SYSINFO_W    300u
#define SYSINFO_H    185u
#define WELCOME_W    240u
#define WELCOME_H    135u
#define CONTROLS_W   160u
#define CONTROLS_H   135u
#define WIN_TOP       70u
#define WIN_GAP       20u

/*  Total content = 300+240+160 = 700,  gaps = 40,  sum = 740
 *  SYSINFO_X  = (800-740)/2 = 30
 *  WELCOME_X  = 30+300+20   = 350
 *  CONTROLS_X = 350+240+20  = 610                              */
#define SYSINFO_X    30u
#define WELCOME_X    350u
#define CONTROLS_X   610u

/* Hint bar: centred at the very bottom, just above the status bar */
#define HINT_Y       544u

/* Window-movement step (pixels per arrow-key press) */
#define MOVE_STEP    8u

/* ── Mutable window positions and focus ─────────────────────── */
static uint32_t wpos_x[3], wpos_y[3];
static int      wfocus;  /* index of the focused window (0-2) */

/* ── Scene helpers ──────────────────────────────────────────── */

static void dahle_draw_desktop(void) {
    gui_desktop();
    gui_statusbar(OS_NAME "  v" OS_VERSION,
                  "Tab  Arrows  ESC  S+ESC");
}

/* ── Window content helpers ─────────────────────────────────── */

/* Draw a line of text inside a window at a logical line index. */
static void dahle_wtext(uint32_t wx, uint32_t wy,
                         uint32_t line, const char *text, uint32_t color) {
    uint32_t x = wx + GUI_PAD;
    uint32_t y = wy + GUI_TITLE_H + 2u + line * (screen_char_h() + LINE_GAP);
    screen_draw_str_px(x, y, text, color, GC_WIN_BG);
}

/* Draw a dim key and a bright value on the same line, aligned to a fixed column. */
static void dahle_wkv(uint32_t wx, uint32_t wy,
                       uint32_t line, const char *key, const char *val) {
    dahle_wtext(wx, wy, line, key, GC_TEXT_DIM);
    uint32_t val_x = wx + GUI_PAD + 9u * screen_char_w();
    uint32_t val_y = wy + GUI_TITLE_H + 2u + line * (screen_char_h() + LINE_GAP);
    screen_draw_str_px(val_x, val_y, val, GC_TEXT, GC_WIN_BG);
}

/* Draw a teal focus-glow border 2 px outside a window's bounding box. */
static void dahle_focus_glow(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (x < 2u || y < 2u) return;
    screen_fill_rounded_rect(x - 2u, y - 2u, w + 4u, h + 4u, 8u, GC_ACCENT);
}

/* ── Pre-built windows ──────────────────────────────────────── */

/* System information — shows live uptime and an ONLINE status badge. */
static void dahle_sysinfo_window(uint32_t x, uint32_t y, int focused) {
    if (focused) dahle_focus_glow(x, y, SYSINFO_W, SYSINFO_H);
    gui_window(x, y, SYSINFO_W, SYSINFO_H, "System Information");
    dahle_wkv(x, y, 0, "OS:     ", OS_NAME " v" OS_VERSION);
    dahle_wkv(x, y, 1, "Arch:   ", "x86  32-bit Protected Mode");
    dahle_wkv(x, y, 2, "Video:  ", "800x600x32  VESA");
    dahle_wkv(x, y, 3, "Timer:  ", "PIT  100 Hz");
    dahle_wkv(x, y, 4, "Shell:  ", "DahleOS interactive shell");

    /* Live uptime — refreshes on every redraw (each keypress) */
    char ubuf[12], ustr[20] = {0};
    int_to_str((int)(timer_ticks() / 100u), ubuf);
    strcat(ustr, ubuf);
    strcat(ustr, " s");
    dahle_wkv(x, y, 5, "Uptime: ", ustr);

    /* Status badge — one visual row below the last kv pair */
    uint32_t badge_y = y + GUI_TITLE_H + 2u + 6u * (screen_char_h() + LINE_GAP);
    gui_badge(x + GUI_PAD, badge_y, "ONLINE", GC_WIN_BG, GC_SUCCESS);
}

/* Welcome / about window. */
static void dahle_welcome_window(uint32_t x, uint32_t y, int focused) {
    if (focused) dahle_focus_glow(x, y, WELCOME_W, WELCOME_H);
    gui_window(x, y, WELCOME_W, WELCOME_H, "Welcome");
    dahle_wtext(x, y, 0, "DahleOS Graphical Shell", GC_TEXT);
    dahle_wtext(x, y, 1, "Powered by VESA framebuffer", GC_TEXT_DIM);
    gui_separator(x + GUI_PAD,
                  y + GUI_TITLE_H + 2u + 2u * (screen_char_h() + LINE_GAP),
                  WELCOME_W - 2u * GUI_PAD);
    dahle_wtext(x, y, 3, "v" OS_VERSION "  x86 Protected Mode", GC_TEXT_DIM);
}

/* Controls reference — keyboard shortcuts for the window manager. */
static void dahle_controls_window(uint32_t x, uint32_t y, int focused) {
    if (focused) dahle_focus_glow(x, y, CONTROLS_W, CONTROLS_H);
    gui_window(x, y, CONTROLS_W, CONTROLS_H, "Controls");
    dahle_wtext(x, y, 0, "Tab    focus", GC_TEXT);
    dahle_wtext(x, y, 1, "Arrows  move", GC_TEXT);
    dahle_wtext(x, y, 2, "ESC     exit", GC_TEXT);
    dahle_wtext(x, y, 3, "S+ESC   apps", GC_TEXT);
}

/* ── Full desktop redraw ──────────────────────────────────────
   Repaints the entire scene from scratch.  Called after every
   input event so the display always reflects current state.   */
static void dahle_redraw(void) {
    dahle_draw_desktop();
    dahle_sysinfo_window (wpos_x[0], wpos_y[0], wfocus == 0);
    dahle_welcome_window (wpos_x[1], wpos_y[1], wfocus == 1);
    dahle_controls_window(wpos_x[2], wpos_y[2], wfocus == 2);
    gui_label_centered(0u, HINT_Y, 800u,
        "Tab  next window  |  Arrows  move  |  ESC  exit  |  Shift+ESC  apps",
        GC_TEXT_DIM, TRANSPARENT);
}

/* ── App helpers ──────────────────────────────────────────────
   Apps render a centred modal over a dark scrim, then block
   on keyboard_getchar() until the user dismisses them.        */

static void dahle_modal_scrim(void) {
    screen_fill_rect(0u, 0u, 800u, 580u, RGB(6, 9, 14));
}

/* App: System Monitor */
static void dahle_app_sysmon(void) {
    const uint32_t AW = 380u, AH = 220u;
    const uint32_t AX = (800u - AW) / 2u;
    const uint32_t AY = (600u - AH) / 2u;

    dahle_modal_scrim();
    gui_window(AX, AY, AW, AH, "System Monitor");

    dahle_wkv(AX, AY, 0, "OS:      ", OS_NAME " v" OS_VERSION);
    dahle_wkv(AX, AY, 1, "Arch:    ", "x86  32-bit Protected Mode");
    dahle_wkv(AX, AY, 2, "Video:   ", "800x600x32  VESA framebuffer");
    dahle_wkv(AX, AY, 3, "Timer:   ", "PIT  100 Hz");

    uint32_t ticks = timer_ticks();
    uint32_t secs  = ticks / 100u;
    uint32_t mins  = secs  / 60u;
    uint32_t hrs   = mins  / 60u;

    char hs[8], ms[8], ss[8], ts[12];
    int_to_str((int)hrs,       hs);
    int_to_str((int)(mins%60), ms);
    int_to_str((int)(secs%60), ss);
    int_to_str((int)ticks,     ts);

    char upstr[32] = {0};
    strcat(upstr, hs); strcat(upstr, "h  ");
    strcat(upstr, ms); strcat(upstr, "m  ");
    strcat(upstr, ss); strcat(upstr, "s");
    dahle_wkv(AX, AY, 4, "Uptime:  ", upstr);
    dahle_wkv(AX, AY, 5, "Ticks:   ", ts);

    uint32_t badge_y = AY + GUI_TITLE_H + 2u + 7u * (screen_char_h() + LINE_GAP);
    gui_badge(AX + GUI_PAD, badge_y, "ONLINE", GC_WIN_BG, GC_SUCCESS);

    gui_label_centered(0u, AY + AH + 14u, 800u,
                       "Press any key to close...", GC_TEXT_DIM, TRANSPARENT);
    keyboard_getchar();
}

/* App: About DahleOS */
static void dahle_app_about(void) {
    const uint32_t AW = 340u, AH = 185u;
    const uint32_t AX = (800u - AW) / 2u;
    const uint32_t AY = (600u - AH) / 2u;

    dahle_modal_scrim();
    gui_window(AX, AY, AW, AH, "About " OS_NAME);
    dahle_wtext(AX, AY, 0, OS_NAME "  v" OS_VERSION, GC_TEXT);
    dahle_wtext(AX, AY, 1, "A 32-bit bare-metal OS", GC_TEXT_DIM);
    gui_separator(AX + GUI_PAD,
                  AY + GUI_TITLE_H + 2u + 2u * (screen_char_h() + LINE_GAP),
                  AW - 2u * GUI_PAD);
    dahle_wtext(AX, AY, 3, "Architecture: x86 Protected Mode", GC_TEXT_DIM);
    dahle_wtext(AX, AY, 4, "Display:  VESA 800x600x32 bpp", GC_TEXT_DIM);
    dahle_wtext(AX, AY, 5, "Runtime:  QEMU i386", GC_TEXT_DIM);

    gui_label_centered(0u, AY + AH + 14u, 800u,
                       "Press any key to close...", GC_TEXT_DIM, TRANSPARENT);
    keyboard_getchar();
}

/* ── App Launcher overlay ──────────────────────────────────── */
static void dahle_launcher(void) {
    const uint32_t LW = 280u, LH = 155u;
    const uint32_t LX = (800u - LW) / 2u;
    const uint32_t LY = (600u - LH) / 2u;

    dahle_modal_scrim();
    gui_window(LX, LY, LW, LH, "App Launcher");
    dahle_wtext(LX, LY, 0, "[1]  System Monitor", GC_TEXT);
    dahle_wtext(LX, LY, 1, "[2]  About " OS_NAME, GC_TEXT);
    gui_separator(LX + GUI_PAD,
                  LY + GUI_TITLE_H + 2u + 2u * (screen_char_h() + LINE_GAP),
                  LW - 2u * GUI_PAD);
    dahle_wtext(LX, LY, 3, "[ESC]  Close", GC_TEXT_DIM);

    char c = keyboard_getchar();
    if      (c == '1') dahle_app_sysmon();
    else if (c == '2') dahle_app_about();
}

/* ── Desktop command ──────────────────────────────────────────

   `dahle` runs an interactive desktop with three movable windows.
   Tab cycles focus; arrow keys nudge the focused window; ESC
   exits; Shift+ESC opens the App Launcher.

   To add a new window: implement a draw function following the
   pattern of dahle_controls_window(), extend wpos_x/wpos_y to
   [4], increment the Tab modulus, and call it from dahle_redraw().
   ─────────────────────────────────────────────────────────── */
static void cmd_dahle(const char *args) {
    (void)args;

    /* Initialise window positions to the centred default layout */
    wpos_x[0] = SYSINFO_X;  wpos_y[0] = WIN_TOP;
    wpos_x[1] = WELCOME_X;  wpos_y[1] = WIN_TOP;
    wpos_x[2] = CONTROLS_X; wpos_y[2] = WIN_TOP;
    wfocus = 0;

    dahle_redraw();

    char c;
    while (1) {
        c = keyboard_getchar();

        if (c == '\x1b') {                        /* ESC key              */
            if (keyboard_shift_held()) {           /*  Shift+ESC: launcher */
                dahle_launcher();
                dahle_redraw();
                continue;
            }
            break;                                 /*  bare ESC: exit      */
        }

        if (c == '\t') {                           /* Tab: cycle focus     */
            wfocus = (wfocus + 1) % 3;
        } else if (c == '\x11') {                  /* ↑                    */
            if (wpos_y[wfocus] >= MOVE_STEP + 2u)
                wpos_y[wfocus] -= MOVE_STEP;
        } else if (c == '\x12') {                  /* ↓                    */
            if (wpos_y[wfocus] + MOVE_STEP < 560u)
                wpos_y[wfocus] += MOVE_STEP;
        } else if (c == '\x13') {                  /* ←                    */
            if (wpos_x[wfocus] >= MOVE_STEP + 2u)
                wpos_x[wfocus] -= MOVE_STEP;
        } else if (c == '\x14') {                  /* →                    */
            if (wpos_x[wfocus] + MOVE_STEP < 798u)
                wpos_x[wfocus] += MOVE_STEP;
        }

        dahle_redraw();
    }

    /* Restore text-mode shell */
    screen_set_color(WHITE, GUI_DESKTOP);
    screen_clear();
    kprint_color("\n  " OS_NAME "  v" OS_VERSION "\n", LGREEN, GUI_DESKTOP);
    kprint_color("  ----------------\n\n", GUI_BORDER, GUI_DESKTOP);
    kprint_color("  All systems nominal.\n\n", GREEN, GUI_DESKTOP);
}






static void cmd_kernelpanic(const char *args) {
    (void)args;

    /* Phase 1: flood screen with random coloured noise (glitch effect)
       screen_noise uses the same LCG as rand_range, seeded from the timer */
    screen_noise(timer_ticks() + 1u);

    /* Phase 2: hold glitch for ~1 second (100 ticks @ 100 Hz) */
    uint32_t t0 = timer_ticks();    
    while (timer_ticks() - t0 < 100);

    /* Phase 3: clear to black */
    screen_set_color(WHITE, BLACK);
    screen_clear();

    /* Phase 4: bomb ASCII art (pure printable ASCII — no CP437) */
    kprint("\n");
    kprint("                     * *\n");
    kprint("          ######    * * *\n");
    kprint("        ##      ##\n");
    kprint("       ##   ()   ##\n");
    kprint("       ##        ##\n");
    kprint("     ##############\n");
    kprint("    ## ########## ##\n");
    kprint("   ## ############ ##\n");
    kprint("   ## ############ ##\n");
    kprint("   ## ############ ##\n");
    kprint("    ## ########## ##\n");
    kprint("     ##############\n");
    kprint("       ##########\n");
    kprint("         ######\n");
    kprint("           ##\n");
    kprint("\n");

    /* Phase 5: panic message */
    kprint_color("  *** KERNEL PANIC: Unhandled exception 0xBRUH ***\n", WHITE, RED);
    kprint_color("  System halted to prevent damage.\n", LGREY, BLACK);
    kprint_color("  Press any key to reboot...\n", LGREY, BLACK);

    /* Phase 6: wait for keypress, then reboot */
    keyboard_getchar();
    cmd_reboot(NULL);
}

static void cmd_repeat(const char *args) {
    if (!args || !*args) {
        kprint("Usage: repeat <n> <cmd>\n");
        kprint("Example: repeat 5 echo Hello\n");
        return;
    }
    // parse the count
    char count_str[12] = {0};
    int i = 0;
    while (args[i] && args[i] != ' ' && i < 11) {
        count_str[i] = args[i];
        i++;
    }

    // skip space
    while (args[i] == ' ') i++;

    // rest is the command + its args
    const char *subcmd = &args[i];

    if (!*subcmd) {
        kprint("Usage: repeat <n> <command>\n");
        return;
    }

    int count = str_to_int(count_str);

    if (count <= 0 || count > 100) {
        kprint("Error: count must be between 1 and 100\n");
        return;
    }

    for (int r = 0; r < count; r++) {
        shell_execute(subcmd);
    }
}

static void cmd_about(const char *args) {
    (void)args;
    kprint("DahleOS ");
    kprint_color(OS_VERSION, CYAN, BLACK);
    kprint("\nBy Dionysios Yi Pei-Chen.\n");
    kprint("- 'A mini operating system not to be official.'\n");
}

/* ================================================================
   FILESYSTEM COMMANDS
   Each wrapper parses the first token from args and delegates to
   the fs layer (fs/fs.c).  All path logic lives there.
   ================================================================ */

/* Extract first whitespace-delimited token from args into out[]. */
static void first_token(const char *args, char *out, int max) {
    int i = 0;
    while (args && args[i] && args[i] != ' ' && i < max - 1) {
        out[i] = args[i]; i++;
    }
    out[i] = '\0';
}

static void cmd_pwd(const char *args)   { (void)args; fs_cmd_pwd(); }
static void cmd_ls (const char *args)   { (void)args; fs_cmd_ls();  }

static void cmd_mkdir(const char *args) {
    char name[FS_MAX_NAME] = {0};
    first_token(args, name, FS_MAX_NAME);
    fs_cmd_mkdir(name[0] ? name : (void *)0);
}

static void cmd_touch(const char *args) {
    char name[FS_MAX_NAME] = {0};
    first_token(args, name, FS_MAX_NAME);
    fs_cmd_touch(name[0] ? name : (void *)0);
}

static void cmd_rm(const char *args) {
    char name[FS_MAX_NAME] = {0};
    first_token(args, name, FS_MAX_NAME);
    fs_cmd_rm(name[0] ? name : (void *)0);
}

static void cmd_cd(const char *args) {
    /* pass full args as path so 'cd /home/tmp' works */
    fs_cmd_cd(args);
}

static void cmd_cat(const char *args) {
    char name[FS_MAX_NAME] = {0};
    first_token(args, name, FS_MAX_NAME);
    fs_cmd_cat(name[0] ? name : (void *)0);
}

/* ================================================================
   ALU  —  arithmetic on decimal or binary (0b…) operands
   ================================================================ */

/* Convert binary string "101" → 5.  Returns -1 on bad input. */
static int bin_to_int(const char *s) {
    int result = 0;
    if (!s || !*s) return -1;
    while (*s) {
        if (*s != '0' && *s != '1') return -1;
        result = (result << 1) | (*s - '0');
        s++;
    }
    return result;
}

/* Convert integer n → null-terminated binary string in buf.
   buf must be at least 33 bytes.  Negative values get a '-' prefix. */
static void int_to_bin(int n, char *buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }

    int neg = 0;
    if (n < 0) { neg = 1; n = -n; }

    char tmp[33];
    int  i = 0;
    unsigned int u = (unsigned int)n;
    while (u) { tmp[i++] = (char)('0' + (u & 1)); u >>= 1; }

    int out = 0;
    if (neg) buf[out++] = '-';
    for (int j = i - 1; j >= 0; j--) buf[out++] = tmp[j];
    buf[out] = '\0';
}

/* Parse a token: "0b101" or "101" treated as binary, else decimal. */
static int parse_operand(const char *s, int *out) {
    if (!s || !*s) return 0;
    if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        int v = bin_to_int(s + 2);
        if (v < 0) return 0;
        *out = v;
        return 1;
    }
    /* decimal (may be negative) */
    *out = str_to_int(s);
    return 1;
}

static void cmd_alu(const char *args) {
    if (!args || !*args) {
        kprint("Usage: alu <a> <b> <+|-|*|/>\n");
        kprint("  Operands can be decimal or binary with 0b prefix.\n");
        kprint("  Example: alu 5 8 +   or   alu 0b0101 0b1000 +\n");
        return;
    }

    /* Parse three whitespace-separated tokens: a, b, op */
    char tok[3][20] = {{0},{0},{0}};
    int  ti = 0, ci = 0;
    const char *p = args;
    while (*p && ti < 3) {
        while (*p == ' ') p++;
        ci = 0;
        while (*p && *p != ' ' && ci < 19) tok[ti][ci++] = *p++;
        tok[ti][ci] = '\0';
        if (ci) ti++;
    }

    if (ti < 3) {
        kprint("Usage: alu <a> <b> <+|-|*|/>\n");
        return;
    }

    int a, b;
    if (!parse_operand(tok[0], &a) || !parse_operand(tok[1], &b)) {
        kprint_color("Error: invalid operand\n", LRED, BLACK);
        return;
    }

    char op = tok[2][0];
    int  result;
    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/':
            if (b == 0) { kprint_color("Error: division by zero\n", LRED, BLACK); return; }
            result = a / b;
            break;
        default:
            kprint_color("Error: unknown operator (use +, -, *, /)\n", LRED, BLACK);
            return;
    }

    /* Print decimal result */
    kprint("=");
    kprint_color_int(result, LGREEN, BLACK);
    kprint("\n");

    /* Also show binary representations */
    char ba[34], bb[34], br[34];
    int_to_bin(a,      ba);
    int_to_bin(b,      bb);
    int_to_bin(result, br);
    kprint("  bin: 0b"); kprint(ba);
    kprint(" "); kprint(tok[2]);
    kprint(" 0b"); kprint(bb);
    kprint(" = 0b"); kprint_color(br, LGREEN, BLACK);
    kprint("\n");
}




static void cmd_upper(const char *args) {
    if (!args || !*args) {
        kprint("Usage: upper <s>\n");
        kprint("Example: upper example -> EXAMPLE\n");
        return;
    }
    char buf[LINE_MAX];
    int i = 0;
    while (args[i] && i < LINE_MAX - 1) {
        char c = args[i];
        if (c >= 'a' && c <= 'z') { // ASCII equivalent of lowercase to uppercase
            c = c - 32;
        }
        buf[i] = c;
        i += 1;
    }
    buf[i] = '\0';

    kprint(buf);
    kprint("\n");
}

static void cmd_lower(const char *args) {
    if (!args || !*args) {
        kprint("Usage: lower <s>\n");
        kprint("Example: lower EXAMPLE -> example\n");
        return;
    }
    char buf[LINE_MAX];
    int i = 0;
    while (args[i] && i < LINE_MAX - 1) {
        char c = args[i];
        if (c >= 'A' && c <= 'Z') { // ASCII equivalent of uppercase to lowercase
            c = c + 32;
        }
        buf[i] = c;
        i += 1;
    }
    buf[i] = '\0';

    kprint(buf);
    kprint("\n");
}

/* Returns the expanded value for a given alias name, or NULL if not found.
   Called by shell.c run() — this is the Option B bridge. */
const char *alias_resolve(const char *name) {
    for (int i = 0; i < alias_count; i++)
        if (strcmp(alias_table[i].name, name) == 0)
            return alias_table[i].value;
    return (void *)0;
}

static void cmd_alias(const char *args) {
    // No args → list all aliases
    if (!args || !*args) {
        if (alias_count == 0) {
            kprint("No aliases defined.\n");
            return;
        }
        for (int i = 0; i < alias_count; i++) {
            kprint_color(alias_table[i].name, LGREEN, BLACK);
            kprint(" -> ");
            kprint(alias_table[i].value);
            kprint("\n");
        }
        return;
    }

    // Parse: alias <name> <value>
    char name[ALIAS_NAME_MAX] = {0};
    int i = 0;
    while (args[i] && args[i] != ' ' && i < ALIAS_NAME_MAX - 1) {
        name[i] = args[i]; i++;
    }
    while (args[i] == ' ') i++;
    const char *value = args + i;

    if (!name[0] || !*value) {
        kprint("Usage: alias <name> <command>\n");
        kprint("       alias              (list all)\n");
        return;
    }

    // Update existing alias if name matches
    for (int a = 0; a < alias_count; a++) {
        if (strcmp(alias_table[a].name, name) == 0) {
            strncpy(alias_table[a].value, value, ALIAS_VAL_MAX - 1);
            kprint("Alias updated.\n");
            return;
        }
    }

    // Add new alias
    if (alias_count >= ALIAS_MAX) {
        kprint_color("Error: alias table full\n", LRED, BLACK);
        return;
    }
    strncpy(alias_table[alias_count].name,  name,  ALIAS_NAME_MAX - 1);
    strncpy(alias_table[alias_count].value, value, ALIAS_VAL_MAX  - 1);
    alias_count++;
    kprint("Alias set.\n");
}