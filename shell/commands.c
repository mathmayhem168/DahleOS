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
   These helpers are the building blocks for the Dahle GUI.
   They operate entirely through gui.h — no raw pixels needed.

   Window content coordinate helper
   ---------------------------------
   To place text or widgets inside a window drawn at (wx, wy):
     content_x = wx + GUI_PAD
     content_y = wy + GUI_TITLE_H + 2 + line * (screen_char_h() + LINE_GAP)
   Use dahle_wtext() / dahle_wkv() instead of computing this manually.
   ============================================================= */

/* Pixels between text lines inside a window */
#define LINE_GAP      4u

/* ── Dahle desktop layout ────────────────────────────────────── */
#define SYSINFO_W    300u
#define SYSINFO_H    148u
#define WELCOME_W    240u
#define WELCOME_H    110u
#define WIN_TOP       60u
#define WIN_GAP       10u     /* horizontal gap between the two windows    */

/* Centre the two-window group on the 800-px screen */
#define SYSINFO_X  ((800u - SYSINFO_W - WIN_GAP - WELCOME_W) / 2u)  /* = 125 */
#define WELCOME_X  (SYSINFO_X + SYSINFO_W + WIN_GAP)                /* = 435 */

/* Elements below the windows */
#define CLOSE_BTN_H   26u
#define CLOSE_BTN_Y  (WIN_TOP + WELCOME_H + 10u)                    /* = 180 */
#define HINT_Y       (CLOSE_BTN_Y + CLOSE_BTN_H + 12u)             /* = 218 */

/* ── Scene helpers ──────────────────────────────────────────── */

/* Draw the full desktop: vertical gradient + status bar.
   Call this first; everything else goes on top. */
static void dahle_draw_desktop(void) {
    gui_desktop();
    gui_statusbar(OS_NAME "  v" OS_VERSION, "dahle  |  Press any key to exit");
}

/* ── Window content helpers ─────────────────────────────────── */

/* Print a line of text inside a window.
   `wx`, `wy`    – top-left corner of the window (same values passed to gui_window).
   `line`        – 0-based line index inside the content area.
   `text`        – the string to draw.
   `color`       – foreground colour (e.g. GC_TEXT or GC_TEXT_DIM). */
static void dahle_wtext(uint32_t wx, uint32_t wy,
                         uint32_t line, const char *text, uint32_t color) {
    uint32_t x = wx + GUI_PAD;
    uint32_t y = wy + GUI_TITLE_H + 2 + line * (screen_char_h() + LINE_GAP);
    screen_draw_str_px(x, y, text, color, GC_WIN_BG);
}

/* Draw a key / value pair on the same line inside a window.
   The key is drawn in dim text; the value follows after a fixed offset. */
static void dahle_wkv(uint32_t wx, uint32_t wy,
                       uint32_t line, const char *key, const char *val) {
    dahle_wtext(wx, wy, line, key, GC_TEXT_DIM);
    /* Value starts after a fixed column so entries align vertically */
    uint32_t val_x = wx + GUI_PAD + 9u * screen_char_w();
    uint32_t val_y = wy + GUI_TITLE_H + 2 + line * (screen_char_h() + LINE_GAP);
    screen_draw_str_px(val_x, val_y, val, GC_TEXT, GC_WIN_BG);
}

/* ── Pre-built windows ──────────────────────────────────────── */

/* System information window */
static void dahle_sysinfo_window(uint32_t x, uint32_t y) {
    gui_window(x, y, SYSINFO_W, SYSINFO_H, "System Information");
    dahle_wkv(x, y, 0, "OS:     ", OS_NAME " v" OS_VERSION);
    dahle_wkv(x, y, 1, "Arch:   ", "x86  32-bit Protected Mode");
    dahle_wkv(x, y, 2, "Video:  ", "800x600x32  VESA");
    dahle_wkv(x, y, 3, "Timer:  ", "PIT  100 Hz");
    dahle_wkv(x, y, 4, "Shell:  ", "DahleOS interactive shell");
}

/* A small welcome / about window */
static void dahle_welcome_window(uint32_t x, uint32_t y) {
    gui_window(x, y, WELCOME_W, WELCOME_H, "Welcome");
    dahle_wtext(x, y, 0, "DahleOS Graphical Shell", GC_TEXT);
    dahle_wtext(x, y, 1, "Powered by VESA framebuffer", GC_TEXT_DIM);
    gui_separator(x + GUI_PAD, y + GUI_TITLE_H + 2 + 2 * (screen_char_h() + LINE_GAP),
                  WELCOME_W - 2 * GUI_PAD);
    dahle_wtext(x, y, 3, "Type 'dahle' to re-open.", GC_TEXT_DIM);
}

/* ── Desktop command ──────────────────────────────────────────

   `dahle`  paints the graphical desktop, draws windows, and
   waits for a keypress before returning to the shell.

   To build your own desktop scene:
     1.  Call  dahle_draw_desktop()  to paint the background.
     2.  Add windows with  gui_window()  or the pre-built helpers.
     3.  Populate content with  dahle_wtext() / dahle_wkv().
     4.  Add buttons with  gui_button(),  badges with  gui_badge().
     5.  Call  keyboard_getchar()  to hold the view until a key
         is pressed, then restore the shell with screen_clear().
   ─────────────────────────────────────────────────────────── */

static void cmd_dahle(const char *args) {
    (void)args;

    /* ── Scene ── */
    dahle_draw_desktop();
    dahle_sysinfo_window(SYSINFO_X, WIN_TOP);
    dahle_welcome_window(WELCOME_X, WIN_TOP);

    /* Close button below the welcome window */
    gui_button(WELCOME_X, CLOSE_BTN_Y, WELCOME_W, CLOSE_BTN_H, "Close (any key)");

    /* Hint centred across the full screen width */
    gui_label_centered(0, HINT_Y, 800u,
                       "Press any key to return to the shell.",
                       GC_TEXT_DIM, TRANSPARENT);

    /* ── Wait ── */
    keyboard_getchar();

    /* ── Restore shell ── */
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