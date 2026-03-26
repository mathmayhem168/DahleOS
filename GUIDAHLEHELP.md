# DahleOS GUI Development Guide
### Everything you need to build real graphical interfaces in `shell/commands.c`

---

## Before you start — what this GUI actually is

DahleOS has **no mouse, no desktop environment, no event loop, and no OS beneath it.**
The "GUI" is a pixel framebuffer: you write coloured pixels directly to video memory.
Every widget (`gui_window`, `gui_button`, …) is just a drawing call — it puts pixels on
screen and forgets about them.  Nothing redraws itself automatically.

Key facts to burn into memory now:

| Fact | Why it matters |
|---|---|
| Screen is **800 × 600 pixels** | Every coordinate must stay inside this range |
| Origin **(0, 0) is the top-left corner** | Y increases **downward**, not upward |
| Input is **keyboard only** — no mouse | Buttons are visual only; you choose them with keys |
| `keyboard_getchar()` **blocks** until a key arrives | Your screen freezes while waiting |
| `keyboard_poll()` returns 1 immediately if a key is ready | Use this for animation / live menus |
| `keyboard_shift_held()` returns 1 while Shift is physically held | Call it right after `getchar` to detect Shift+key chords |
| Arrow keys return `KEY_UP/DOWN/LEFT/RIGHT` (`\x11`–`\x14`) | Extended PS/2 scancodes are now mapped automatically |
| **Nothing erases itself** — redraw over it to erase | Paint the background colour on top |
| Character cells are **8 px wide, 16 px tall** | `screen_char_w()` = 8, `screen_char_h()` = 16 |

---

## Table of Contents

1. [The Coordinate System](#lesson-1-the-coordinate-system)
2. [Drawing Shapes](#lesson-2-drawing-shapes)
3. [The Colour System](#lesson-3-the-colour-system)
4. [Your First Window](#lesson-4-your-first-window)
5. [Placing Content Inside Windows](#lesson-5-placing-content-inside-windows)
6. [Buttons and Badges](#lesson-6-buttons-and-badges)
7. [Input and Interactivity](#lesson-7-input-and-interactivity)
8. [Key Shortcuts — the real way](#lesson-8-key-shortcuts--the-real-way)
9. [State Tracking — getting data from the GUI](#lesson-9-state-tracking--getting-data-from-the-gui)
10. [macOS-style Window Buttons](#lesson-10-macos-style-window-buttons)
11. [Multi-window Desktops](#lesson-11-multi-window-desktops)
12. [Advanced Patterns and Full Example](#lesson-12-advanced-patterns-and-full-example)
13. [The Dahle Window Manager — the real desktop](#lesson-13-the-dahle-window-manager--the-real-desktop)

---

## Lesson 1: The Coordinate System

### The canvas

```
(0,0) ──────────────────────────── (799,0)
  │                                    │
  │    x increases →                   │
  │    y increases ↓                   │
  │                                    │
(0,599) ─────────────────────────(799,599)
```

Every drawing call takes `(x, y, width, height)`.
`x` = pixels from the **left edge**.
`y` = pixels from the **top edge**.

### Screen constants

Always use these instead of hardcoding 800/600:

```c
screen_px_w()   /* → 800 */
screen_px_h()   /* → 600 */
screen_char_w() /* → 8   (one character = 8 px wide)  */
screen_char_h() /* → 16  (one character = 16 px tall) */
```

### Centring something on screen

```c
uint32_t win_w = 300;
uint32_t win_h = 200;
uint32_t win_x = (screen_px_w() - win_w) / 2;  /* = 250 */
uint32_t win_y = (screen_px_h() - win_h) / 2;  /* = 200 */
```

### ⚠️ Common positioning mistakes

```c
/* WRONG — goes off screen right: 750 + 200 = 950 > 800 */
gui_window(750, 100, 200, 150, "Too far right");

/* WRONG — negative result because window is wider than screen */
uint32_t x = (screen_px_w() - 900) / 2;  /* uint32_t underflows! becomes ~2 billion */

/* RIGHT — always check: x + w <= screen_px_w() */
uint32_t w = 300;
uint32_t x = (screen_px_w() - w) / 2;   /* safe */
```

---

## Lesson 2: Drawing Shapes

All drawing functions are in `drivers/screen.h`.  Include `drivers/gui.h` and you get
everything automatically.

### Filled rectangle

```c
screen_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
```

Example — draw a 100×50 red block at (50, 80):
```c
screen_fill_rect(50, 80, 100, 50, GC_DANGER);  /* GC_DANGER = red */
```

### Rounded rectangle

```c
screen_fill_rounded_rect(uint32_t x, uint32_t y,
                          uint32_t w, uint32_t h,
                          uint32_t radius, uint32_t color);
```

`radius` is the corner curve radius in pixels.

```c
/* Small radius = barely rounded */
screen_fill_rounded_rect(100, 100, 200, 60, 3, GC_WIN_BG);

/* Large radius = very rounded (pill-shaped when radius = h/2) */
screen_fill_rounded_rect(100, 100, 200, 30, 15, GC_ACCENT);

/* Perfect circle: width == height, radius == width/2 */
screen_fill_rounded_rect(200, 200, 20, 20, 10, GC_SUCCESS);  /* green circle */
```

> **Rule of thumb for radius:** `radius ≤ width/2` and `radius ≤ height/2`.
> If you exceed that, the engine clamps it automatically, but you get unexpected shapes.

### Horizontal gradient

```c
screen_fill_gradient_h(uint32_t x, uint32_t y,
                        uint32_t w, uint32_t h,
                        uint32_t col_left, uint32_t col_right);
```

Fades from `col_left` on the left edge to `col_right` on the right edge.

```c
/* Blue-to-dark banner */
screen_fill_gradient_h(0, 0, screen_px_w(), 40, GC_ACCENT, GC_WIN_BG);
```

### Vertical gradient

```c
screen_fill_gradient_v(uint32_t x, uint32_t y,
                        uint32_t w, uint32_t h,
                        uint32_t col_top, uint32_t col_bottom);
```

```c
/* Desktop background — fades dark-blue to near-black top-to-bottom */
gui_desktop();           /* does exactly this internally */

/* Custom gradient in a region */
screen_fill_gradient_v(50, 50, 300, 200, GC_ACCENT, GC_WIN_BG);
```

### Drawing text at exact pixel positions

```c
screen_draw_str_px(uint32_t x, uint32_t y, const char *s,
                   uint32_t fg, uint32_t bg);

screen_draw_char_px(uint32_t x, uint32_t y, char c,
                    uint32_t fg, uint32_t bg);
```

Pass `TRANSPARENT` as `bg` to skip background pixels (text floats over a gradient):

```c
screen_draw_str_px(20, 50, "Hello!", GC_TEXT, TRANSPARENT);
```

> **Do NOT use `kprint` / `kprint_color` inside a GUI scene.**
> Those functions use the text cursor (`cur_col`, `cur_row`), which is the scrolling
> text layer.  Inside a GUI command always use `screen_draw_str_px` or `gui_label`.

---

## Lesson 3: The Colour System

### RGB macro — build any colour

```c
uint32_t color = RGB(red, green, blue);  /* each value 0–255 */

uint32_t orange  = RGB(255, 140,  0);
uint32_t sky     = RGB(135, 206, 235);
uint32_t mid_grey = RGB(128, 128, 128);
```

### screen.h palette — for text/terminal colours

```c
BLACK    WHITE    RED      GREEN   BLUE
CYAN     MAGENTA  BROWN    LGREY   DGREY
LBLUE    LGREEN   LCYAN    LRED    LMAGENTA  LBROWN
```

### gui.h theme constants — for GUI elements (use these in GUI scenes)

```c
GC_DESKTOP_TOP   /* dark navy — top of the desktop gradient     */
GC_DESKTOP_BOT   /* near-black — bottom of desktop gradient     */
GC_WIN_BG        /* window body background                      */
GC_WIN_TITLE     /* title bar teal  RGB(0,120,140)              */
GC_WIN_BORDER    /* window / panel border                       */
GC_BTN           /* button background                           */
GC_BTN_BORDER    /* button border (cyan)  RGB(0,180,180)        */
GC_TEXT          /* primary text (near-white)                   */
GC_TEXT_DIM      /* dim text (grey)                             */
GC_SEP           /* separator line colour                       */
GC_STATUSBAR     /* status bar background                       */
GC_ACCENT        /* accent cyan  RGB(0,180,180)                 */
GC_DANGER        /* red — errors, warnings, destructive actions */
GC_SUCCESS       /* green — OK, confirmed, success              */
GC_WARN          /* yellow — caution / warning                  */
```

### Special value: TRANSPARENT

```c
#define TRANSPARENT  0xFF000000u
```

When passed as `bg` to any text-drawing or label function, background pixels are skipped.
Use this whenever you draw text over a gradient or image.

```c
/* Text over desktop gradient — no black boxes */
gui_label(20, 200, "Floating text", GC_TEXT, TRANSPARENT);

/* Text with a solid cell background */
gui_label(20, 200, "Solid bg text", GC_TEXT, GC_WIN_BG);
```

---

## Lesson 4: Your First Window

```c
void gui_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                const char *title);
```

### What gui_window draws (from top to bottom)

```
y - 0   ┌──────────────────────────────────┐  ← border top (GC_WIN_BORDER)
y + 1   │  ████████ TITLE BAR ████████████ │  ← title bar (GC_WIN_TITLE), height = GUI_TITLE_H (22px)
y + 23  ├──────────────────────────────────┤  ← separator line (1px)
y + 24  │                                  │
        │        CONTENT AREA              │  ← window body (GC_WIN_BG)
        │                                  │
y + h-1 └──────────────────────────────────┘  ← border bottom

Also: a drop shadow drawn 4px right / 5px down from the window.
```

### Minimum sensible window sizes

```c
/* These values ensure the content area isn't crushed */
#define GUI_TITLE_H  22u   /* title bar height */
#define GUI_PAD      10u   /* padding inside content area */

/* Minimum height: title bar + separator + top pad + 1 row of text + bottom pad */
/* = 1 + 22 + 1 + 16 + 10 = 50 px. In practice use ≥ 80 for anything useful. */
```

### Exercise 4-A: Draw a centred window

```c
static void cmd_mywindow(const char *args) {
    (void)args;

    /* 1. Paint the desktop background */
    gui_desktop();

    /* 2. Draw the window */
    uint32_t W = 320, H = 160;
    uint32_t wx = (screen_px_w() - W) / 2;   /* centred horizontally */
    uint32_t wy = (screen_px_h() - H) / 2;   /* centred vertically   */
    gui_window(wx, wy, W, H, "My First Window");

    /* 3. Wait for a key, then return to the shell */
    keyboard_getchar();
    screen_set_color(WHITE, GUI_DESKTOP);
    screen_clear();
}
```

### ⚠️ Common window mistakes

```c
/* MISTAKE 1 — window placed so shadow goes off screen */
gui_window(796, 0, 100, 100, "Oops");
/* Shadow tries to draw at x=800, y=5 → clipped (harmless but ugly).
   Keep at least 10 px from right and bottom edges. */

/* MISTAKE 2 — window too tall for screen */
gui_window(50, 50, 300, 600, "Too tall");
/* 50 + 600 = 650 > 600. Bottom gets clipped. */

/* MISTAKE 3 — drawing content above the title bar */
uint32_t wy = 100;
screen_draw_str_px(10, wy + 5, "I'm in the title!", GC_TEXT, GC_WIN_BG);
/* You're painting over the title bar. Content starts at wy + GUI_TITLE_H + 2. */
```

---

## Lesson 5: Placing Content Inside Windows

The content area starts below the title bar and separator:

```
content_y_start = wy + 1 + GUI_TITLE_H + 1   =   wy + GUI_TITLE_H + 2
content_x_start = wx + GUI_PAD                =   wx + 10
```

### The line-based helper pattern

The existing `dahle_wtext` pattern (from `cmd_dahle`) is the simplest way to place
multiple lines inside a window.  You can copy and reuse it:

```c
/* LINE_GAP is the extra vertical spacing between text lines */
#define LINE_GAP  4u

/* Draw one line of text at line index `line` (0-based) inside a window */
static void my_wtext(uint32_t wx, uint32_t wy,
                     uint32_t line, const char *text, uint32_t color) {
    uint32_t x = wx + GUI_PAD;
    uint32_t y = wy + GUI_TITLE_H + 2 + line * (screen_char_h() + LINE_GAP);
    screen_draw_str_px(x, y, text, color, GC_WIN_BG);
}

/* Use it: */
gui_window(wx, wy, 300, 200, "Info");
my_wtext(wx, wy, 0, "Line 0 — top line",  GC_TEXT);
my_wtext(wx, wy, 1, "Line 1 — below it",  GC_TEXT_DIM);
my_wtext(wx, wy, 2, "Line 2",             GC_SUCCESS);
```

Each line number adds `screen_char_h() + LINE_GAP = 16 + 4 = 20 pixels` to Y.

### Fitting content: how many lines fit in a window?

```c
uint32_t H = 200;
uint32_t content_h = H - GUI_TITLE_H - 2 - GUI_PAD;  /* subtract header + bottom pad */
uint32_t lines_fit  = content_h / (screen_char_h() + LINE_GAP);
/* For H=200: content_h = 200-22-2-10=166, lines_fit = 166/20 = 8 lines */
```

### Displaying a number inside a window

There is no `printf`. Use `int_to_str` from `libc/string.h`:

```c
#include "../libc/string.h"

char buf[12];
int score = 42;
int_to_str(score, buf);          /* buf = "42" */
my_wtext(wx, wy, 0, buf, GC_TEXT);
```

For hex values use `uint_to_hex`:
```c
char buf[11];
uint_to_hex(0xDEADBEEF, buf);   /* buf = "0xDEADBEEF" */
my_wtext(wx, wy, 1, buf, GC_TEXT);
```

To concatenate two strings (like a label + value):
```c
char buf[64];
strcpy(buf, "Score: ");
char num[12];
int_to_str(score, num);
strcat(buf, num);                /* buf = "Score: 42" */
my_wtext(wx, wy, 0, buf, GC_TEXT);
```

### gui_separator — visual divider inside windows

```c
gui_separator(wx + GUI_PAD,
              wy + GUI_TITLE_H + 2 + 2 * (screen_char_h() + LINE_GAP),
              W - 2 * GUI_PAD);
/* Places a 1-px horizontal line across the content area, after 2 lines of text */
```

### gui_panel — a window without a title bar

```c
gui_panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
```

Useful for sub-areas inside a larger scene — a sidebar, a data box, etc.

```c
/* A sidebar on the left side of the screen */
gui_panel(10, 10, 180, 540);

/* Place text inside it (no title bar offset — content starts at y + GUI_PAD) */
screen_draw_str_px(10 + GUI_PAD, 10 + GUI_PAD, "Sidebar", GC_TEXT, GC_WIN_BG);
```

---

## Lesson 6: Buttons and Badges

### gui_button

```c
void gui_button(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                const char *label);
```

Draws a rectangle with a blue accent border and a centred label.
There is **no click handler** — a button is a visual element only.
You decide what "pressing" a button means through keyboard input (see Lesson 7).

```c
/* Minimum readable button size: at least char_h + 8 px tall, label_width + 20 px wide */
gui_button(wx + GUI_PAD, wy + GUI_TITLE_H + 80, 100, 28, "OK");
gui_button(wx + GUI_PAD + 110, wy + GUI_TITLE_H + 80, 100, 28, "Cancel");
```

### gui_badge

```c
void gui_badge(uint32_t x, uint32_t y, const char *text,
               uint32_t fg, uint32_t bg);
```

A small pill-shaped label — good for status chips, counts, tags.
Size is computed automatically from the text length.

```c
gui_badge(wx + GUI_PAD, wy + GUI_TITLE_H + 40, "ONLINE",  GC_WIN_BG, GC_SUCCESS);
gui_badge(wx + GUI_PAD, wy + GUI_TITLE_H + 60, "WARNING", GC_WIN_BG, GC_WARN);
gui_badge(wx + GUI_PAD, wy + GUI_TITLE_H + 80, "ERROR",   GC_WIN_BG, GC_DANGER);
```

### "Highlighted" / selected button state

Because buttons are just drawn pixels, you create a selected appearance by drawing the
button in a different colour using raw drawing calls:

```c
/* Normal button */
static void draw_button_normal(uint32_t x, uint32_t y,
                                uint32_t w, uint32_t h, const char *label) {
    gui_button(x, y, w, h, label);  /* accent-bordered, dark bg */
}

/* Selected / active button — bright background */
static void draw_button_selected(uint32_t x, uint32_t y,
                                  uint32_t w, uint32_t h, const char *label) {
    screen_fill_rounded_rect(x, y, w, h, 4, GC_ACCENT);              /* bright fill */
    screen_draw_str_px(x + (w - (uint32_t)strlen(label) * 8) / 2,
                       y + (h - 16) / 2,
                       label, GC_WIN_BG, TRANSPARENT);                /* dark text */
}
```

Call `draw_button_normal` or `draw_button_selected` based on your state variable
(covered in detail in Lesson 9).

---

## Lesson 7: Input and Interactivity

### The two input functions

```c
char keyboard_getchar(void);   /* BLOCKS — waits until a key is pressed  */
int  keyboard_poll(void);      /* returns 1 immediately if key is waiting */
```

### Pattern 1 — Wait for any key (simplest)

Use this when you just want "press any key to continue":

```c
gui_window(wx, wy, 300, 120, "Done");
my_wtext(wx, wy, 0, "Operation complete.", GC_TEXT);
my_wtext(wx, wy, 1, "Press any key...",    GC_TEXT_DIM);
keyboard_getchar();   /* blocks here */
/* continues after any keypress */
```

### Pattern 2 — Menu driven by key choices

```c
static void cmd_menu(const char *args) {
    (void)args;
    gui_desktop();

    uint32_t wx = 200, wy = 150, W = 400, H = 250;
    gui_window(wx, wy, W, H, "Main Menu");
    my_wtext(wx, wy, 0, "[1]  System Information",  GC_TEXT);
    my_wtext(wx, wy, 1, "[2]  Run Diagnostics",     GC_TEXT);
    my_wtext(wx, wy, 2, "[3]  About DahleOS",        GC_TEXT);
    my_wtext(wx, wy, 4, "[ESC] Return to shell",    GC_TEXT_DIM);

    while (1) {
        char c = keyboard_getchar();

        if (c == '1') {
            /* draw a different screen ... */
        } else if (c == '2') {
            /* ... */
        } else if (c == '\x1B') {  /* ESC = 0x1B = 27 */
            break;                  /* exit the GUI */
        }
        /* Any unrecognised key: ignore and loop back */
    }

    screen_set_color(WHITE, GUI_DESKTOP);
    screen_clear();
}
```

> **ESC** always returns `'\x1B'` (hex 27).  This is the conventional "go back" key
> across all screens in DahleOS.

### Pattern 3 — Non-blocking poll loop (live / animated UIs)

When you need to update the screen repeatedly (clock, counter, animation) WITHOUT
blocking on a keypress, use `keyboard_poll()`:

```c
while (1) {
    /* Redraw something that changes each iteration */
    char tbuf[12];
    int_to_str((int)timer_ticks(), tbuf);
    screen_fill_rect(wx + GUI_PAD, wy + GUI_TITLE_H + 40, 120, 16, GC_WIN_BG);  /* erase old */
    screen_draw_str_px(wx + GUI_PAD, wy + GUI_TITLE_H + 40, tbuf, GC_TEXT, GC_WIN_BG);

    /* Check for input without blocking */
    if (keyboard_poll()) {
        char c = keyboard_getchar();
        if (c == '\x1B') break;
    }
}
```

> **Important:** `timer_ticks()` is from `cpu/timer.h` — include it if you need time.
> It counts at 100 Hz (100 ticks per second).

### Key values reference

| Key | Value | How to check |
|---|---|---|
| ESC | 27 (0x1B) | `c == '\x1B'` or `c == 27` |
| Enter | `'\n'` | `c == '\n'` |
| Backspace | `'\b'` | `c == '\b'` |
| Tab | `'\t'` | `c == '\t'` |
| Space | `' '` | `c == ' '` |
| `a`–`z` | `'a'`–`'z'` | `c == 'a'` etc. |
| `A`–`Z` | `'A'`–`'Z'` (requires Shift held) | `c == 'A'` etc. |
| `0`–`9` | `'0'`–`'9'` | `c == '0'` etc. |
| Arrow Up | `KEY_UP` (`'\x11'`) | `c == KEY_UP` |
| Arrow Down | `KEY_DOWN` (`'\x12'`) | `c == KEY_DOWN` |
| Arrow Left | `KEY_LEFT` (`'\x13'`) | `c == KEY_LEFT` |
| Arrow Right | `KEY_RIGHT` (`'\x14'`) | `c == KEY_RIGHT` |

`KEY_UP/DOWN/LEFT/RIGHT` are defined in `drivers/keyboard.h` — include that header.

> ⚠️ **The Ctrl key is NOT tracked by the current keyboard driver.**
> F-keys and Home/End are also discarded — use letter/number keys for those shortcuts.

---

## Lesson 8: Key Shortcuts — the real way

### Simple single-key shortcuts

In a GUI event loop, `keyboard_getchar()` returns one character at a time.
Map characters to actions:

```c
while (1) {
    char c = keyboard_getchar();
    switch (c) {
    case '\x1B': goto done;          /* ESC — exit */
    case 'w':    close_active();     break;  /* W — close window */
    case 'n':    open_new_window();  break;  /* N — new window   */
    case 'h':    show_help_screen(); break;  /* H — help         */
    case '+':    zoom_in();          break;
    case '-':    zoom_out();         break;
    }
}
done:
    screen_set_color(WHITE, GUI_DESKTOP);
    screen_clear();
```

### Shift is available — double your shortcuts

The keyboard driver tracks Shift in two complementary ways:

**1. Letter case** — lowercase `w` and uppercase `W` are different characters:

```c
case 'w': /* close window     */  break;
case 'W': /* close ALL windows */ break;   /* Shift+W */
```

**2. `keyboard_shift_held()` — for chords like Shift+ESC**

ESC and arrow keys produce the same character regardless of Shift, so you must call
`keyboard_shift_held()` immediately after `keyboard_getchar()` to detect those chords:

```c
while (1) {
    char c = keyboard_getchar();

    if (c == '\x1B') {
        if (keyboard_shift_held()) {
            open_launcher();   /* Shift+ESC */
        } else {
            break;             /* bare ESC — exit */
        }
        continue;
    }

    if (c == KEY_UP && keyboard_shift_held()) {
        move_fast_up();    /* Shift+Up — bigger jump */
    } else if (c == KEY_UP) {
        move_one_step();   /* bare Up */
    }
}
```

> **Timing:** `keyboard_shift_held()` reads the Shift state at the instant you call it.
> Always call it immediately after `keyboard_getchar()`, before any other input call.

### "Modifier" simulation with a prefix key

Since Ctrl doesn't work, you can simulate a "command mode" with a toggle key:

```c
int cmd_mode = 0;   /* 0 = normal typing, 1 = command mode */

while (1) {
    char c = keyboard_getchar();

    if (c == '\t') {          /* Tab toggles command mode */
        cmd_mode = !cmd_mode;
        redraw_mode_indicator(cmd_mode);
        continue;
    }

    if (cmd_mode) {
        /* Tab was pressed before this key — treat as shortcut */
        switch (c) {
        case 'w': close_window(); break;
        case 'n': new_window();   break;
        case 'q': goto done;      break;
        }
        cmd_mode = 0;   /* auto-exit command mode after one action */
    } else {
        /* Normal character — use it for something else */
    }
}
```

### Displaying a shortcut hint in the status bar

```c
gui_statusbar("DahleOS  Desktop", "N=New  W=Close  ESC=Exit");
```

The status bar is always at the bottom of the screen and shows short instructions.

---

## Lesson 9: State Tracking — getting data from the GUI

This is the most important lesson.  Because drawing functions are one-way (no return
value, no callback), **you must store all state yourself in variables**.

### Tracking window position

```c
/* Store where your window is so you can redraw, move, or query it */
uint32_t win_x = 100, win_y = 80;
uint32_t win_w = 320, win_h = 200;

gui_window(win_x, win_y, win_w, win_h, "My Window");

/* Later — where does the content area start? */
uint32_t content_x = win_x + GUI_PAD;
uint32_t content_y = win_y + GUI_TITLE_H + 2;

/* Is a point "inside" this window? (useful for hit-testing concepts) */
int inside_x = (query_x >= win_x) && (query_x < win_x + win_w);
int inside_y = (query_y >= win_y) && (query_y < win_y + win_h);
```

### Tracking which item is selected in a menu

```c
#define ITEM_COUNT 4
static const char *items[ITEM_COUNT] = {
    "New File", "Open File", "Save File", "Quit"
};

static void draw_menu(uint32_t wx, uint32_t wy, int selected) {
    for (int i = 0; i < ITEM_COUNT; i++) {
        uint32_t iy = wy + GUI_TITLE_H + 2 + (uint32_t)i * 24;
        if (i == selected) {
            /* Highlight bar */
            screen_fill_rect(wx + 2, iy - 2, win_w - 4, 20, GC_ACCENT);
            screen_draw_str_px(wx + GUI_PAD, iy, items[i], GC_WIN_BG, TRANSPARENT);
        } else {
            /* Clear the row (erase highlight if it was there before) */
            screen_fill_rect(wx + 2, iy - 2, win_w - 4, 20, GC_WIN_BG);
            screen_draw_str_px(wx + GUI_PAD, iy, items[i], GC_TEXT, GC_WIN_BG);
        }
    }
}
```

Navigate with:
```c
int sel = 0;
draw_menu(wx, wy, sel);

while (1) {
    char c = keyboard_getchar();
    if (c == 'j' || c == 's') { if (sel < ITEM_COUNT-1) sel++; }
    if (c == 'k' || c == 'w') { if (sel > 0)            sel--; }
    if (c == '\n') { /* execute items[sel] */ break; }
    if (c == '\x1B') break;
    draw_menu(wx, wy, sel);     /* redraw with new selection */
}
```

### Tracking a toggle (checkbox-like state)

```c
int dark_mode = 0;   /* 0 = light, 1 = dark */

/* Draw the toggle */
static void draw_toggle(uint32_t x, uint32_t y, int state) {
    uint32_t track_color = state ? GC_SUCCESS : GC_WIN_BORDER;
    screen_fill_rounded_rect(x, y, 34, 14, 7, track_color);   /* track */
    uint32_t knob_x = state ? x + 20 : x + 2;
    screen_fill_rounded_rect(knob_x, y + 2, 10, 10, 5, GC_TEXT);  /* knob */
}

/* In your event loop: */
case 'd':
    dark_mode = !dark_mode;
    /* Erase old toggle area, redraw it */
    screen_fill_rect(toggle_x - 2, toggle_y - 2, 40, 18, GC_WIN_BG);
    draw_toggle(toggle_x, toggle_y, dark_mode);
    break;
```

### Tracking a numeric value (counter / slider)

```c
int counter = 0;

static void redraw_counter(uint32_t wx, uint32_t wy, int val) {
    /* Erase the old value by painting background over it */
    screen_fill_rect(wx + GUI_PAD, wy + GUI_TITLE_H + 40, 120, 16, GC_WIN_BG);
    /* Draw the new value */
    char buf[12];
    int_to_str(val, buf);
    screen_draw_str_px(wx + GUI_PAD, wy + GUI_TITLE_H + 40, buf, GC_TEXT, GC_WIN_BG);
}

/* In your event loop: */
case '+': counter++; redraw_counter(wx, wy, counter); break;
case '-': counter--; redraw_counter(wx, wy, counter); break;
```

### Storing the state of multiple windows

```c
typedef struct {
    uint32_t x, y, w, h;
    int      visible;
    const char *title;
} window_t;

window_t wins[4] = {
    { 50,  50, 300, 200, 0, "Window A" },
    { 400, 80, 250, 180, 0, "Window B" },
    { 100, 300, 320, 150, 0, "Window C" },
    { 420, 300, 200, 120, 0, "Window D" },
};

/* Draw only the visible windows */
static void draw_all_windows(void) {
    gui_desktop();
    for (int i = 0; i < 4; i++) {
        if (wins[i].visible)
            gui_window(wins[i].x, wins[i].y, wins[i].w, wins[i].h, wins[i].title);
    }
    gui_statusbar("DahleOS", "1-4=Toggle  ESC=Exit");
}
```

---

## Lesson 10: macOS-style Window Buttons

macOS windows have three coloured circles in the title bar: close (red), minimise
(yellow), maximise (green).  Since `gui_window` doesn't draw these automatically,
you add them manually AFTER calling `gui_window`.

```c
/* Draw three macOS-style traffic-light buttons on top of an already-drawn window */
static void draw_traffic_lights(uint32_t wx, uint32_t wy) {
    /* Vertical centre of title bar */
    uint32_t ty = wy + 1 + (GUI_TITLE_H - 10) / 2;   /* = wy + 7 */
    uint32_t d  = 10;   /* circle diameter */
    uint32_t r  = d / 2;

    /* Red — Close (leftmost, 8px from border) */
    screen_fill_rounded_rect(wx + 8, ty, d, d, r, GC_DANGER);

    /* Yellow — Minimise (20px from red) */
    screen_fill_rounded_rect(wx + 8 + d + 6, ty, d, d, r, GC_WARN);

    /* Green — Maximise (20px from yellow) */
    screen_fill_rounded_rect(wx + 8 + 2*(d + 6), ty, d, d, r, GC_SUCCESS);
}

/* Usage: always call AFTER gui_window so circles appear on top of the title bar */
uint32_t wx = 100, wy = 80, W = 400, H = 250;
gui_window(wx, wy, W, H, "My App");
draw_traffic_lights(wx, wy);
```

### Making traffic-light buttons do something

Since there is no mouse, key shortcuts map to button actions:

```c
int win_open = 1;    /* tracks if this window is "open" */
int win_min  = 0;    /* tracks if this window is "minimised" */

while (win_open) {
    char c = keyboard_getchar();
    switch (c) {
    case 'x':   /* red — close */
        win_open = 0;
        break;
    case 'm':   /* yellow — minimise (toggle) */
        win_min = !win_min;
        if (win_min) {
            /* Redraw window as a thin title-bar-only strip */
            gui_desktop();
            gui_window(wx, wy, W, GUI_TITLE_H + 2, "My App (minimised)");
            draw_traffic_lights(wx, wy);
        } else {
            /* Restore full window */
            gui_desktop();
            gui_window(wx, wy, W, H, "My App");
            draw_traffic_lights(wx, wy);
            /* … redraw content … */
        }
        break;
    case 'f':   /* green — maximise (toggle) */
        /* Store original size and go full screen */
        /* … see Lesson 11 for a full pattern … */
        break;
    case '\x1B':
        win_open = 0;
        break;
    }
}
```

### Showing the key hint inside the title bar text itself

```c
gui_window(wx, wy, 400, 250, "My App  [X=close M=min F=max]");
```

Embedding hints in the title keeps them discoverable without a separate label.

---

## Lesson 11: Multi-window Desktops

### Pattern — number keys open/close independent windows

```c
#define N_WINS  3

typedef struct {
    uint32_t x, y, w, h;
    int      open;
    const char *title;
} Win;

static Win wins[N_WINS] = {
    { 30,  40, 300, 180, 0, "System Info" },
    { 380, 40, 250, 200, 0, "File Browser" },
    { 150, 280, 320, 160, 0, "Settings" },
};

static void draw_scene(void) {
    gui_desktop();
    for (int i = 0; i < N_WINS; i++) {
        if (wins[i].open) {
            gui_window(wins[i].x, wins[i].y,
                       wins[i].w, wins[i].h, wins[i].title);
            draw_traffic_lights(wins[i].x, wins[i].y);
        }
    }
    gui_statusbar("DahleOS  Desktop",
                  "1=SysInfo  2=Files  3=Settings  ESC=Exit");
}

static void cmd_desktop(const char *args) {
    (void)args;
    draw_scene();

    while (1) {
        char c = keyboard_getchar();
        if (c >= '1' && c <= '3') {
            int i = c - '1';          /* '1'→0, '2'→1, '3'→2 */
            wins[i].open = !wins[i].open;   /* toggle */
            draw_scene();             /* full redraw */
        } else if (c == '\x1B') {
            break;
        }
    }

    screen_set_color(WHITE, GUI_DESKTOP);
    screen_clear();
}
```

> **Full redraw** is always safe and simple.  Because the entire desktop and all
> windows are repainted in under 1 ms, there is no visible flicker.  Only optimise
> to partial redraws if you have a measurable reason to.

### Pattern — window with multiple "pages" (Tab to cycle)

```c
int page = 0;   /* which page we are on */
#define PAGES 3

static void draw_page(uint32_t wx, uint32_t wy, int p) {
    /* Erase the content area */
    screen_fill_rect(wx + 1, wy + GUI_TITLE_H + 2,
                     W - 2, H - GUI_TITLE_H - 3, GC_WIN_BG);

    if (p == 0) {
        my_wtext(wx, wy, 0, "Page 1 — Overview",   GC_TEXT);
        my_wtext(wx, wy, 1, "Some data here...",   GC_TEXT_DIM);
    } else if (p == 1) {
        my_wtext(wx, wy, 0, "Page 2 — Details",    GC_TEXT);
    } else {
        my_wtext(wx, wy, 0, "Page 3 — Advanced",   GC_TEXT);
    }

    /* Page indicator at the bottom of the window */
    char indicator[16] = "Page X / 3";
    indicator[5] = '1' + p;
    screen_draw_str_px(wx + GUI_PAD,
                       wy + H - GUI_PAD - screen_char_h(),
                       indicator, GC_TEXT_DIM, GC_WIN_BG);
}

/* In event loop: */
case '\t':
    page = (page + 1) % PAGES;
    draw_page(wx, wy, page);
    break;
```

### Pattern — moving a window with arrow keys (preferred) or WASD

Arrow keys now return `KEY_UP / KEY_DOWN / KEY_LEFT / KEY_RIGHT` from `keyboard.h`.
Use them for window movement; they feel natural and don't conflict with typing:

```c
#include "../drivers/keyboard.h"   /* KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT */

#define MOVE_STEP 8u   /* pixels per keypress */

while (1) {
    char c = keyboard_getchar();
    int moved = 1;

    if      (c == KEY_UP    && win_y >= MOVE_STEP + 2u) win_y -= MOVE_STEP;
    else if (c == KEY_DOWN  && win_y + MOVE_STEP < 560u) win_y += MOVE_STEP;
    else if (c == KEY_LEFT  && win_x >= MOVE_STEP + 2u) win_x -= MOVE_STEP;
    else if (c == KEY_RIGHT && win_x + MOVE_STEP < 798u) win_x += MOVE_STEP;
    else if (c == '\x1B') break;
    else moved = 0;

    if (moved) {
        gui_desktop();
        gui_window(win_x, win_y, win_w, win_h, "Moving Window");
        gui_statusbar("Move Mode", "Arrows=Move  ESC=Exit");
    }
}
```

You can also use WASD if you prefer letter keys (useful when arrows are reserved for
something else in the same loop):

```c
if      (c == 'a' && win_x > MOVE_STEP) win_x -= MOVE_STEP;
else if (c == 'd' && win_x + win_w + MOVE_STEP < screen_px_w()) win_x += MOVE_STEP;
else if (c == 'w' && win_y > MOVE_STEP) win_y -= MOVE_STEP;
else if (c == 's' && win_y + win_h + MOVE_STEP < screen_px_h()) win_y += MOVE_STEP;
```

---

## Lesson 12: Advanced Patterns and Full Example

### Reading back the "state" of a button

There is no built-in way to read whether a button was pressed — you maintain an `int`
variable that represents the button's logical state, and you update it in response to
keypresses.

```c
/* Two-option choice — OK or Cancel */
int result = -1;   /* -1 = not yet chosen, 0 = cancel, 1 = ok */

gui_window(wx, wy, 300, 130, "Confirm Action");
my_wtext(wx, wy, 0, "Delete everything?", GC_TEXT);
gui_button(wx + GUI_PAD,        wy + GUI_TITLE_H + 60, 100, 28, "OK  [O]");
gui_button(wx + GUI_PAD + 110,  wy + GUI_TITLE_H + 60, 100, 28, "Cancel [C]");

while (result == -1) {
    char c = keyboard_getchar();
    if (c == 'o' || c == 'O' || c == '\n') result = 1;
    if (c == 'c' || c == 'C' || c == '\x1B') result = 0;
}

if (result == 1) {
    /* user chose OK — do the destructive thing */
} else {
    /* user cancelled */
}
```

### Displaying dynamic data — uptime counter on a panel

```c
#include "../cpu/timer.h"

static void draw_uptime_panel(uint32_t px, uint32_t py) {
    uint32_t t = timer_ticks();
    uint32_t s = t / 100, m = s / 60, h = m / 60;

    char line[48];
    strcpy(line, "Uptime: ");
    char num[12];
    int_to_str((int)h,       num); strcat(line, num); strcat(line, "h ");
    int_to_str((int)(m % 60),num); strcat(line, num); strcat(line, "m ");
    int_to_str((int)(s % 60),num); strcat(line, num); strcat(line, "s");

    /* Erase old text, draw new */
    screen_fill_rect(px, py, 200, 16, GC_WIN_BG);
    screen_draw_str_px(px, py, line, GC_SUCCESS, GC_WIN_BG);
}
```

### Full working example: a complete multi-feature desktop

This is a complete `cmd_` function you can paste into `shell/commands.c`.
It demonstrates every concept from this guide:

```c
/* ─── Full GUI Desktop Example ────────────────────────────────────────
   Add to cmd_table:  { "gui2", "Advanced GUI demo", cmd_gui2 },
   ──────────────────────────────────────────────────────────────────── */

#define G2_LINE_GAP  4u

/* Content line helper — same pattern as dahle_wtext */
static void g2_text(uint32_t wx, uint32_t wy, uint32_t line,
                    const char *txt, uint32_t col) {
    screen_draw_str_px(wx + GUI_PAD,
                       wy + GUI_TITLE_H + 2 + line * (screen_char_h() + G2_LINE_GAP),
                       txt, col, GC_WIN_BG);
}

/* Draw three macOS circles on a window */
static void g2_traffic(uint32_t wx, uint32_t wy) {
    uint32_t ty = wy + 1 + (GUI_TITLE_H - 10) / 2;
    screen_fill_rounded_rect(wx + 8,      ty, 10, 10, 5, GC_DANGER);
    screen_fill_rounded_rect(wx + 24,     ty, 10, 10, 5, GC_WARN);
    screen_fill_rounded_rect(wx + 40,     ty, 10, 10, 5, GC_SUCCESS);
}

/* The full scene */
static void g2_draw(int sysopen, int aboutopen, int sel, int counter) {
    gui_desktop();

    /* Status bar */
    gui_statusbar("DahleOS  GUI2",
                  "1=SysInfo  2=About  J/K=Select  +/-=Count  ESC=Exit");

    /* ── Window 1: System info (toggleable) ── */
    if (sysopen) {
        uint32_t wx = 40, wy = 50, W = 320, H = 200;
        gui_window(wx, wy, W, H, "System Information");
        g2_traffic(wx, wy);
        g2_text(wx, wy, 0, "OS:      DahleOS v1.0.0",  GC_TEXT);
        g2_text(wx, wy, 1, "Arch:    x86 32-bit PM",   GC_TEXT_DIM);
        g2_text(wx, wy, 2, "Video:   800x600x32 VESA", GC_TEXT_DIM);
        gui_separator(wx + GUI_PAD,
                      wy + GUI_TITLE_H + 2 + 3 * (screen_char_h() + G2_LINE_GAP),
                      W - 2 * GUI_PAD);

        /* Live uptime */
        uint32_t t = timer_ticks() / 100;
        char ubuf[24];
        strcpy(ubuf, "Uptime: ");
        char nb[12]; int_to_str((int)t, nb);
        strcat(ubuf, nb); strcat(ubuf, "s");
        g2_text(wx, wy, 4, ubuf, GC_SUCCESS);
    }

    /* ── Window 2: About (toggleable) ── */
    if (aboutopen) {
        uint32_t wx = 420, wy = 80, W = 320, H = 180;
        gui_window(wx, wy, W, H, "About");
        g2_traffic(wx, wy);
        g2_text(wx, wy, 0, "DahleOS Graphical Shell",  GC_TEXT);
        g2_text(wx, wy, 1, "Powered by VESA LFB",      GC_TEXT_DIM);
        gui_badge(wx + GUI_PAD, wy + GUI_TITLE_H + 2 + 2 * (screen_char_h() + G2_LINE_GAP),
                  "v1.0.0", GC_WIN_BG, GC_ACCENT);
    }

    /* ── Floating menu panel ── */
    {
        uint32_t px = 200, py = 340, PW = 380, PH = 110;
        gui_panel(px, py, PW, PH);

        static const char *opts[3] = { "Option Alpha", "Option Beta", "Option Gamma" };
        for (int i = 0; i < 3; i++) {
            uint32_t ry = py + GUI_PAD + (uint32_t)i * 26;
            if (i == sel) {
                screen_fill_rect(px + 2, ry - 2, PW - 4, 20, GC_ACCENT);
                screen_draw_str_px(px + GUI_PAD, ry, opts[i], GC_WIN_BG, TRANSPARENT);
            } else {
                screen_draw_str_px(px + GUI_PAD, ry, opts[i], GC_TEXT, GC_WIN_BG);
            }
        }

        /* Counter display on the right side of the panel */
        char cbuf[16];
        strcpy(cbuf, "Count: ");
        char cn[12]; int_to_str(counter, cn);
        strcat(cbuf, cn);
        screen_draw_str_px(px + PW - (uint32_t)strlen(cbuf) * screen_char_w() - GUI_PAD,
                           py + GUI_PAD + 30,
                           cbuf, GC_WARN, GC_WIN_BG);
    }
}

static void cmd_gui2(const char *args) {
    (void)args;

    int sysopen   = 1;
    int aboutopen = 1;
    int sel       = 0;
    int counter   = 0;

    g2_draw(sysopen, aboutopen, sel, counter);

    while (1) {
        char c = keyboard_getchar();

        if      (c == '1') { sysopen   = !sysopen; }
        else if (c == '2') { aboutopen = !aboutopen; }
        else if ((c == 'j' || c == 's') && sel < 2) { sel++; }
        else if ((c == 'k' || c == 'w') && sel > 0) { sel--; }
        else if (c == '+' || c == '=')  { counter++; }
        else if (c == '-')              { counter--; }
        else if (c == '\x1B')           { break; }
        else                            { continue; }  /* unknown key — skip redraw */

        g2_draw(sysopen, aboutopen, sel, counter);
    }

    screen_set_color(WHITE, GUI_DESKTOP);
    screen_clear();
}
```

**To add this command to DahleOS:**
1. Paste the three static helpers (`g2_text`, `g2_traffic`, `g2_draw`) and `cmd_gui2`
   into `shell/commands.c`, before the existing `cmd_dahle`.
2. Add `static void cmd_gui2(const char *args);` to the forward declarations section.
3. Add one line to `cmd_table[]`:
   ```c
   { "gui2", "Advanced GUI demo", cmd_gui2 },
   ```
4. Run `make clean && make run`.

---

## Lesson 13: The Dahle Window Manager — the real desktop

This lesson documents the actual `dahle` command as it exists in `shell/commands.c`.
It is the reference implementation for every pattern taught above.

### Overview

`dahle` paints three movable windows on a teal-accented desktop.  The focused window
receives a teal glow border.  An App Launcher opens over a dark modal scrim.

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  Desktop  (gui_desktop gradient + gui_statusbar)                             │
│                                                                              │
│  ┌──────────────────────┐  ┌────────────────────┐  ┌─────────────────┐      │
│  │ System Information   │  │ Welcome            │  │ Controls        │      │
│  │ OS:     DahleOS …    │  │ DahleOS GUI Shell  │  │ Tab    focus    │      │
│  │ Arch:   x86 32-bit   │  │ Powered by VESA…   │  │ Arrows  move   │      │
│  │ Video:  800x600x32   │  │ ─────────────────  │  │ ESC     exit   │      │
│  │ Timer:  PIT 100 Hz   │  │ v1.0.0  x86 PM     │  │ S+ESC   apps  │      │
│  │ Shell:  interactive  │  └────────────────────┘  └─────────────────┘      │
│  │ Uptime: 42 s         │                                                    │
│  │ [ONLINE]             │                                                    │
│  └──────────────────────┘                                                    │
│                                                                              │
│       Tab  next window  |  Arrows  move  |  ESC  exit  |  Shift+ESC  apps   │
├──────────────────────────────────────────────────────────────────────────────┤
│  DahleOS  v1.0.0                               Tab  Arrows  ESC  S+ESC      │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Window layout constants

```c
/* Three windows, centred on the 800-px screen:
 *   SYSINFO  300 px  |  gap 20  |  WELCOME  240 px  |  gap 20  |  CONTROLS  160 px
 *   Total = 740 px   →   left margin = (800-740)/2 = 30 px                        */
#define SYSINFO_W    300u    /* System Information window width  */
#define SYSINFO_H    185u    /* System Information window height */
#define WELCOME_W    240u    /* Welcome window width             */
#define WELCOME_H    135u    /* Welcome window height            */
#define CONTROLS_W   160u    /* Controls reference window width  */
#define CONTROLS_H   135u    /* Controls reference window height */
#define WIN_TOP       70u    /* Y of all three windows           */
#define WIN_GAP       20u    /* horizontal gap between windows   */
#define SYSINFO_X     30u
#define WELCOME_X    350u
#define CONTROLS_X   610u
#define MOVE_STEP      8u    /* pixels moved per arrow keypress  */
#define HINT_Y       544u    /* Y of the bottom hint label       */
```

### Mutable state (file-scope statics)

```c
static uint32_t wpos_x[3], wpos_y[3];  /* current position of each window */
static int      wfocus;                 /* index 0-2 of the focused window  */
```

### Focus glow

The focused window gets a 2 px teal (`GC_ACCENT`) rounded-rect border drawn
**before** `gui_window`, so the window itself renders on top of the glow:

```c
static void dahle_focus_glow(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (x < 2u || y < 2u) return;   /* guard against uint32_t underflow   */
    screen_fill_rounded_rect(x - 2u, y - 2u, w + 4u, h + 4u, 8u, GC_ACCENT);
}

/* Inside each window draw function: */
static void dahle_sysinfo_window(uint32_t x, uint32_t y, int focused) {
    if (focused) dahle_focus_glow(x, y, SYSINFO_W, SYSINFO_H);
    gui_window(x, y, SYSINFO_W, SYSINFO_H, "System Information");
    /* ... content ... */
}
```

### Full desktop redraw

Every input event triggers a full repaint.  This is cheap (< 1 ms) and keeps the
code simple — no dirty-rectangle tracking needed.

```c
static void dahle_redraw(void) {
    dahle_draw_desktop();
    dahle_sysinfo_window (wpos_x[0], wpos_y[0], wfocus == 0);
    dahle_welcome_window (wpos_x[1], wpos_y[1], wfocus == 1);
    dahle_controls_window(wpos_x[2], wpos_y[2], wfocus == 2);
    gui_label_centered(0u, HINT_Y, 800u,
        "Tab  next window  |  Arrows  move  |  ESC  exit  |  Shift+ESC  apps",
        GC_TEXT_DIM, TRANSPARENT);
}
```

### Event loop

```c
char c;
while (1) {
    c = keyboard_getchar();

    if (c == '\x1b') {
        if (keyboard_shift_held()) {   /* Shift+ESC → launcher */
            dahle_launcher();
            dahle_redraw();
            continue;
        }
        break;                         /* bare ESC → exit      */
    }

    if      (c == '\t')    { wfocus = (wfocus + 1) % 3; }
    else if (c == KEY_UP   && wpos_y[wfocus] >= MOVE_STEP + 2u) wpos_y[wfocus] -= MOVE_STEP;
    else if (c == KEY_DOWN && wpos_y[wfocus] + MOVE_STEP < 560u) wpos_y[wfocus] += MOVE_STEP;
    else if (c == KEY_LEFT && wpos_x[wfocus] >= MOVE_STEP + 2u) wpos_x[wfocus] -= MOVE_STEP;
    else if (c == KEY_RIGHT&& wpos_x[wfocus] + MOVE_STEP < 798u) wpos_x[wfocus] += MOVE_STEP;

    dahle_redraw();
}
```

### Modal scrim + App Launcher

Apps dim the screen with a solid dark rect (no alpha blending), then draw a centred
window on top.  This creates the "lights dim" modal effect:

```c
static void dahle_modal_scrim(void) {
    screen_fill_rect(0u, 0u, 800u, 580u, RGB(6, 9, 14));
}

static void dahle_launcher(void) {
    const uint32_t LW = 280u, LH = 155u;
    const uint32_t LX = (800u - LW) / 2u;
    const uint32_t LY = (600u - LH) / 2u;

    dahle_modal_scrim();
    gui_window(LX, LY, LW, LH, "App Launcher");
    dahle_wtext(LX, LY, 0, "[1]  System Monitor",  GC_TEXT);
    dahle_wtext(LX, LY, 1, "[2]  About DahleOS",   GC_TEXT);
    gui_separator(LX + GUI_PAD,
                  LY + GUI_TITLE_H + 2u + 2u * (screen_char_h() + LINE_GAP),
                  LW - 2u * GUI_PAD);
    dahle_wtext(LX, LY, 3, "[ESC]  Close",         GC_TEXT_DIM);

    char c = keyboard_getchar();
    if      (c == '1') dahle_app_sysmon();
    else if (c == '2') dahle_app_about();
}
```

### Adding a 4th window

1. Write `dahle_mywindow(uint32_t x, uint32_t y, int focused)` following the pattern
   of `dahle_controls_window`.
2. Extend `wpos_x` and `wpos_y` to `[4]` and add `wpos_x[3] = MY_X; wpos_y[3] = MY_Y;`
   in the initialisation block of `cmd_dahle`.
3. Add `dahle_mywindow(wpos_x[3], wpos_y[3], wfocus == 3);` to `dahle_redraw()`.
4. Change `wfocus = (wfocus + 1) % 3` → `% 4` in the event loop.
5. Run `make`.

### Live uptime inside a window

Read `timer_ticks()` at draw time — it refreshes automatically each keypress:

```c
char ubuf[12], ustr[20] = {0};
int_to_str((int)(timer_ticks() / 100u), ubuf);   /* ticks ÷ 100 = seconds */
strcat(ustr, ubuf);
strcat(ustr, " s");
dahle_wkv(x, y, 5, "Uptime: ", ustr);
```

---

## Quick Reference Card

### Content Y formula (the one formula you use constantly)

```
content_line_y(win_y, line) = win_y + GUI_TITLE_H + 2 + line * (16 + LINE_GAP)
```
where `LINE_GAP` is your own `#define` (typically 4).

### Input cheat sheet

```c
/* Blocking read — use in event loops */
char c = keyboard_getchar();

/* Non-blocking — use for animation */
if (keyboard_poll()) { char c = keyboard_getchar(); /* ... */ }

/* Shift held right now (call immediately after getchar) */
int shifted = keyboard_shift_held();

/* Arrow key checks */
if (c == KEY_UP)    { /* move up    */ }
if (c == KEY_DOWN)  { /* move down  */ }
if (c == KEY_LEFT)  { /* move left  */ }
if (c == KEY_RIGHT) { /* move right */ }

/* Shift+ESC chord */
if (c == '\x1b' && keyboard_shift_held()) { /* launcher */ }

/* Plain ESC */
if (c == '\x1b' && !keyboard_shift_held()) { /* exit */ }
```

### Shapes cheat sheet

```c
/* Solid rect */
screen_fill_rect(x, y, w, h, color);

/* Rounded rect (circle: w==h, r==w/2) */
screen_fill_rounded_rect(x, y, w, h, r, color);

/* Gradient left→right */
screen_fill_gradient_h(x, y, w, h, left_color, right_color);

/* Gradient top→bottom */
screen_fill_gradient_v(x, y, w, h, top_color, bot_color);

/* Text (pixel-exact) */
screen_draw_str_px(x, y, "text", fg, bg_or_TRANSPARENT);

/* Widget: window */
gui_window(x, y, w, h, "Title");

/* Widget: panel */
gui_panel(x, y, w, h);

/* Widget: button */
gui_button(x, y, w, h, "Label");

/* Widget: badge */
gui_badge(x, y, "Label", fg, bg);

/* Widget: separator */
gui_separator(x, y, width);

/* Widget: status bar */
gui_statusbar("Left text", "Right text");

/* Desktop background */
gui_desktop();
```

### Erase a region (overdraw with background)

```c
screen_fill_rect(x, y, w, h, GC_WIN_BG);     /* inside a window */
screen_fill_rect(x, y, w, h, GC_DESKTOP_BOT); /* on the bare desktop */
```

### Command exit template (always do this before returning)

```c
/* At the end of every cmd_ GUI function: */
screen_set_color(WHITE, GUI_DESKTOP);
screen_clear();
```

---

## Summary of what is and isn't available

| Feature | Status |
|---|---|
| Mouse / pointer input | ❌ Not implemented |
| Text input fields | ❌ Not built-in — `keyboard_getchar` returns one char at a time; build your own buffer |
| Ctrl+key shortcuts | ❌ Ctrl state is not tracked by the keyboard driver |
| Arrow keys | ✅ Supported — return `KEY_UP/DOWN/LEFT/RIGHT` (`'\x11'`–`'\x14'`) |
| Shift key (held state) | ✅ Supported — `keyboard_shift_held()` returns 1 while Shift is physically held |
| F-keys, Home, End | ❌ Extended scancodes not mapped for those keys |
| Alpha transparency | ❌ No blending — `TRANSPARENT` skips background pixels only; it does not blend |
| Automatic redraws | ❌ You must call drawing functions manually |
| Widget callbacks | ❌ No callback system — handle input in your own event loop |
| Window focus / glow | ✅ Supported via `dahle_focus_glow()` pattern (see Lesson 13) |
| App launcher / modal | ✅ Supported via `dahle_modal_scrim()` + overlay window pattern |

---

*This guide covers everything currently built into DahleOS as of v1.0.0.*
*All examples compile with `-m32 -ffreestanding -Werror` under `i686-elf-gcc`.*
