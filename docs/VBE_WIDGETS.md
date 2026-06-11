# VBE Widget Reference — DahleOS

How to draw windows, gradients, buttons, and the four new controls inside a VBE
framebuffer command.  This document is a **reference**, not a tutorial — if you are
new to the GUI system, read `GUI_GUIDE.md` first.

---

## VBE mode — the context every GUI command runs in

DahleOS has two display modes.  The terminal uses VGA hardware text mode (0xB8000,
80×25 characters).  Every GUI command uses VBE — a linear 800×600×32 framebuffer.

The switch is managed automatically:

```c
static void cmd_myapp(const char *args) {
    (void)args;

    /* The framework does this for you at the start of cmd_dahle.
       For your own commands, call it explicitly at the top: */
    screen_enter_vbe();

    /* --- draw here --- */

    /* On exit, restore the VGA text shell */
    screen_enter_vga();
}
```

`screen_enter_vbe()` clears the framebuffer and enables the Bochs VBE card.
`screen_enter_vga()` disables VBE and returns the CRT to 80×25 text mode.

The user can also trigger the switch manually at the shell prompt:

```
graphics set vbe        # switch to VBE framebuffer
graphics set vga        # switch back to VGA text terminal
graphics status         # print the active mode
```

### Canvas dimensions

After `screen_enter_vbe()`:

| Function | Value | Meaning |
|---|---|---|
| `screen_px_w()` | 800 | framebuffer width in pixels |
| `screen_px_h()` | 600 | framebuffer height in pixels |
| `screen_char_w()` | 8 | one character cell, pixels wide |
| `screen_char_h()` | 16 | one character cell, pixels tall |

Always call these instead of writing the literals `800` or `600`.

### Coordinate origin

```
(0,0) ──────────────────── (799,0)
  │   x →                     │
  │   y ↓                     │
(0,599) ─────────────────(799,599)
```

The reserved area at the bottom is the status bar:

```
usable_height = screen_px_h() - GUI_SB_H   /* = 580 px */
```

---

## Windows

```c
#include "../drivers/gui.h"

void gui_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                const char *title);
```

Draws a rounded window with a teal title bar, a 1-px separator, a body, and a
drop shadow offset 4 px right / 5 px down.

```
y + 0    ┌─────────────────────────────┐  ← border (GC_WIN_BORDER, 1 px)
y + 1    │ ████ TITLE BAR ████████████ │  ← GC_WIN_TITLE, GUI_TITLE_H = 22 px
y + 23   ├─────────────────────────────┤  ← separator (1 px)
y + 24   │                             │
         │   content area              │  ← GC_WIN_BG
         │                             │
y + h-1  └─────────────────────────────┘
```

### Content area formula

```c
uint32_t cx = wx + GUI_PAD;               /* = wx + 10   */
uint32_t cy = wy + GUI_TITLE_H + 2u;      /* = wy + 24   */
```

### Line-based placement

```c
#define LINE_GAP 4u   /* extra vertical spacing between rows */

/* Row n inside window (wx, wy): */
uint32_t row_y = wy + GUI_TITLE_H + 2u + n * (screen_char_h() + LINE_GAP);
/*                                           = wy + 24 + n * 20              */
```

### Centring a window

```c
uint32_t W = 360, H = 220;
uint32_t wx = (screen_px_w() - W) / 2u;   /* 220 */
uint32_t wy = (screen_px_h() - H) / 2u;   /* 190 */
gui_window(wx, wy, W, H, "Centred Window");
```

### Safe area guards

```c
/* Shadow goes 5 px below, 4 px right — leave at least 10 px margin */
uint32_t x_max = screen_px_w() - w - 10u;
uint32_t y_max = screen_px_h() - GUI_SB_H - h - 10u;
if (wx > x_max) wx = x_max;
if (wy > y_max) wy = y_max;
```

### Panel — window without a title bar

```c
void gui_panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
```

Same rounded border + body, no title bar, no shadow.  Content starts at
`(x + GUI_PAD, y + GUI_PAD)`.

---

## Gradients

Two gradient directions are available.  Both interpolate R, G, B independently.

### Vertical gradient

```c
void screen_fill_gradient_v(uint32_t x, uint32_t y,
                             uint32_t w, uint32_t h,
                             uint32_t col_top, uint32_t col_bottom);
```

`gui_desktop()` uses this to paint the desktop background:

```c
void gui_desktop(void) {
    screen_fill_gradient_v(0, 0, screen_px_w(), screen_px_h(),
                           GC_DESKTOP_TOP, GC_DESKTOP_BOT);
}
```

Custom usage:

```c
/* Dark-navy to near-black panel, full screen width */
screen_fill_gradient_v(0, 0, screen_px_w(), 80, GC_ACCENT, GC_WIN_BG);

/* Teal-to-dark banner across a window title area */
screen_fill_gradient_v(wx + 1, wy + 1, w - 2, GUI_TITLE_H,
                       GC_WIN_TITLE, GC_WIN_BG);
```

### Horizontal gradient

```c
void screen_fill_gradient_h(uint32_t x, uint32_t y,
                             uint32_t w, uint32_t h,
                             uint32_t col_left, uint32_t col_right);
```

```c
/* Accent-to-dark header strip */
screen_fill_gradient_h(0, 0, screen_px_w(), 36,
                       GC_ACCENT, GC_WIN_BG);

/* Danger-to-warning inside a notification bar */
screen_fill_gradient_h(wx + GUI_PAD, wy + GUI_TITLE_H + 60,
                       W - 2 * GUI_PAD, 14,
                       GC_DANGER, GC_WARN);
```

### Drawing text over a gradient

Pass `TRANSPARENT` as the background to skip background pixel writes:

```c
screen_fill_gradient_h(0, 100, screen_px_w(), 30, GC_ACCENT, GC_WIN_BG);
screen_draw_str_px(20, 107, "Text on gradient", GC_TEXT, TRANSPARENT);
```

Without `TRANSPARENT`, each character gets a solid black box behind it.

---

## Buttons

### Standard button

```c
void gui_button(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                const char *label);
```

Draws a rounded rectangle with a 1-px cyan border (`GC_BTN_BORDER`), a dark body
(`GC_BTN`), and the label centred inside.

```c
gui_button(wx + GUI_PAD,        wy + GUI_TITLE_H + 80, 110, 28, "Confirm [Y]");
gui_button(wx + GUI_PAD + 120u, wy + GUI_TITLE_H + 80, 110, 28, "Cancel  [N]");
```

Minimum readable size: `h ≥ screen_char_h() + 8` and `w ≥ strlen(label) * 8 + 20`.

### Button visual states

Buttons are pure pixels — there is no click handler.  Recreate states by painting
with different colours:

```c
/* Inactive / normal */
gui_button(x, y, w, h, label);

/* Active / selected — bright filled background */
screen_fill_rounded_rect(x, y, w, h, 4u, GC_ACCENT);
uint32_t lw = (uint32_t)strlen(label) * screen_char_w();
screen_draw_str_px(x + (w - lw) / 2u, y + (h - screen_char_h()) / 2u,
                   label, GC_WIN_BG, TRANSPARENT);

/* Disabled — muted colours */
screen_fill_rounded_rect(x, y, w, h, 4u, GC_WIN_BORDER);
uint32_t lw2 = (uint32_t)strlen(label) * screen_char_w();
screen_draw_str_px(x + (w - lw2) / 2u, y + (h - screen_char_h()) / 2u,
                   label, GC_TEXT_DIM, TRANSPARENT);
```

### Event loop pattern for button menus

```c
int choice = -1;   /* -1 = nothing chosen yet */

gui_window(wx, wy, W, H, "Confirm");
gui_label(wx + GUI_PAD, wy + GUI_TITLE_H + 12, "Delete all files?", GC_TEXT, GC_WIN_BG);
gui_button(wx + GUI_PAD,        wy + GUI_TITLE_H + 60, 100, 28, "Yes [Y]");
gui_button(wx + GUI_PAD + 110u, wy + GUI_TITLE_H + 60, 100, 28, "No  [N]");

while (choice < 0) {
    char c = keyboard_getchar();
    if (c == 'y' || c == 'Y' || c == '\n') choice = 1;
    if (c == 'n' || c == 'N' || c == '\x1b') choice = 0;
}
```

---

## New controls

All four controls are in `drivers/gui.h` and `drivers/gui.c`.

### Progress bar

```c
void gui_progress_bar(uint32_t x, uint32_t y, uint32_t w,
                      uint32_t pct, uint32_t color);
```

| Parameter | Meaning |
|---|---|
| `x, y` | top-left corner |
| `w` | total width of the track |
| `pct` | fill percentage, **0–100** (clamped automatically) |
| `color` | fill colour — use `GC_SUCCESS`, `GC_WARN`, `GC_DANGER`, or `GC_ACCENT` |

Fixed height: `GUI_PROGRESS_H = 14 px`.

```c
/* A loading bar at 67 % */
gui_progress_bar(wx + GUI_PAD, wy + GUI_TITLE_H + 40,
                 W - 2 * GUI_PAD, 67, GC_ACCENT);

/* CPU usage at 90 % — warn with red */
gui_progress_bar(wx + GUI_PAD, wy + GUI_TITLE_H + 70,
                 W - 2 * GUI_PAD, 90, GC_DANGER);
```

**Animating a progress bar:**

```c
uint32_t progress = 0;
while (progress <= 100) {
    /* Erase old bar, draw new value */
    screen_fill_rect(wx + GUI_PAD, wy + GUI_TITLE_H + 40,
                     W - 2 * GUI_PAD, GUI_PROGRESS_H, GC_WIN_BG);
    gui_progress_bar(wx + GUI_PAD, wy + GUI_TITLE_H + 40,
                     W - 2 * GUI_PAD, progress, GC_ACCENT);

    /* Busy-wait ~10 ms per step (1 PIT tick at 100 Hz) */
    uint32_t t0 = timer_ticks();
    while (timer_ticks() - t0 < 1u) {}

    if (keyboard_poll()) break;   /* let ESC abort */
    progress++;
}
```

---

### Checkbox

```c
void gui_checkbox(uint32_t x, uint32_t y, const char *label, int checked);
```

| Parameter | Meaning |
|---|---|
| `x, y` | top-left of the 14×14 box |
| `label` | text drawn to the right (may be `""` for no label) |
| `checked` | 0 = empty box, non-zero = filled box |

Box side: `GUI_CHECK_SZ = 14 px`.
Total height equals `GUI_CHECK_SZ` (14 px); allow `GUI_CHECK_SZ + 4` for row spacing.

```c
int dark_mode = 0;
int sound_on  = 1;

gui_checkbox(wx + GUI_PAD, wy + GUI_TITLE_H + 30, "Dark mode",   dark_mode);
gui_checkbox(wx + GUI_PAD, wy + GUI_TITLE_H + 50, "Sound",       sound_on);
gui_checkbox(wx + GUI_PAD, wy + GUI_TITLE_H + 70, "Auto-update", 0);

/* Toggle in the event loop: */
case 'd':
    dark_mode = !dark_mode;
    /* Erase the row, redraw with new state */
    screen_fill_rect(wx + GUI_PAD, wy + GUI_TITLE_H + 28,
                     W - 2 * GUI_PAD, GUI_CHECK_SZ + 4, GC_WIN_BG);
    gui_checkbox(wx + GUI_PAD, wy + GUI_TITLE_H + 30, "Dark mode", dark_mode);
    break;
```

---

### Toggle switch

```c
void gui_toggle(uint32_t x, uint32_t y, int on);
```

| Parameter | Meaning |
|---|---|
| `x, y` | top-left of the 34×14 pill |
| `on` | 0 = grey track (off), non-zero = green track (on) |

Dimensions: `GUI_TOGGLE_W × GUI_TOGGLE_H = 34 × 14 px`.

```c
int wifi = 1;

/* Label on the left, toggle on the right */
screen_draw_str_px(wx + GUI_PAD, wy + GUI_TITLE_H + 32,
                   "Wi-Fi", GC_TEXT, GC_WIN_BG);
gui_toggle(wx + GUI_PAD + 80u, wy + GUI_TITLE_H + 30, wifi);

/* Toggle in the event loop: */
case 'w':
    wifi = !wifi;
    screen_fill_rect(wx + GUI_PAD + 78, wy + GUI_TITLE_H + 28,
                     GUI_TOGGLE_W + 4, GUI_TOGGLE_H + 4, GC_WIN_BG);
    gui_toggle(wx + GUI_PAD + 80u, wy + GUI_TITLE_H + 30, wifi);
    break;
```

**Difference from checkbox:** a toggle visually suggests an immediate binary action
(on/off); a checkbox better suits a persistent option in a settings list.  Both are
purely visual — the semantic difference is in how you use them in your code.

---

### Input field

```c
void gui_input_field(uint32_t x, uint32_t y, uint32_t w,
                     const char *text, int focused);
```

| Parameter | Meaning |
|---|---|
| `x, y` | top-left of the field |
| `w` | total width including borders |
| `text` | current buffer contents (pass `""` for empty, never NULL) |
| `focused` | 0 = muted border; non-zero = accent border + cursor |

Fixed height: `screen_char_h() + 8 = 24 px`.

If the text is wider than the inner area, the field automatically shows the trailing
portion so the cursor stays visible.

**Collecting keyboard input into a buffer:**

```c
static char buf[64];
static int  len = 0;

static void draw_field(uint32_t x, uint32_t y, uint32_t w) {
    screen_fill_rect(x, y, w, screen_char_h() + 8u, GC_WIN_BG);
    gui_input_field(x, y, w, buf, 1 /* focused */);
}

/* In your event loop: */
char c = keyboard_getchar();
if (c == '\b' && len > 0) {
    buf[--len] = '\0';
    draw_field(fx, fy, fw);
} else if (c == '\n') {
    /* process buf */
} else if ((unsigned char)c >= 32 && (unsigned char)c < 127 && len < 63) {
    buf[len++] = c;
    buf[len]   = '\0';
    draw_field(fx, fy, fw);
}
```

**Full example — a search dialog:**

```c
static void cmd_search(const char *args) {
    (void)args;
    screen_enter_vbe();
    gui_desktop();

    const uint32_t W = 360, H = 140;
    const uint32_t wx = (screen_px_w() - W) / 2u;
    const uint32_t wy = (screen_px_h() - H) / 2u;
    const uint32_t fw = W - 2u * GUI_PAD;   /* field width */
    const uint32_t fx = wx + GUI_PAD;
    const uint32_t fy = wy + GUI_TITLE_H + 30u;

    gui_window(wx, wy, W, H, "Search");
    gui_label(wx + GUI_PAD, wy + GUI_TITLE_H + 10,
              "Enter query:", GC_TEXT_DIM, GC_WIN_BG);
    gui_input_field(fx, fy, fw, "", 1);
    gui_button(wx + GUI_PAD, wy + GUI_TITLE_H + 80, 80, 26, "Go [Enter]");

    char buf[64] = {0};
    int  len = 0;

    while (1) {
        char c = keyboard_getchar();
        if (c == '\x1b') break;
        if (c == '\n')   { /* use buf */ break; }
        if (c == '\b' && len > 0) { buf[--len] = '\0'; }
        else if ((unsigned char)c >= 32 && len < 63) { buf[len++] = c; buf[len] = '\0'; }

        /* Erase and redraw the field */
        screen_fill_rect(fx, fy, fw, screen_char_h() + 8u, GC_WIN_BG);
        gui_input_field(fx, fy, fw, buf, 1);
    }

    screen_enter_vga();
}
```

---

## Layout arithmetic cheat sheet

```
Window body top:        wy + GUI_TITLE_H + 2              (= wy + 24)
Row n top:              wy + GUI_TITLE_H + 2 + n * 20     (line_gap = 4)
Content left edge:      wx + GUI_PAD                      (= wx + 10)
Content right edge:     wx + W - GUI_PAD                  (= wx + W - 10)
Status bar top:         screen_px_h() - GUI_SB_H          (= 580)
Hint label Y:           screen_px_h() - GUI_SB_H - screen_char_h() - 20   (= 544)
```

## Size constants

| Constant | Value | Control |
|---|---|---|
| `GUI_TITLE_H` | 22 | window title bar height |
| `GUI_PAD` | 10 | standard inner padding |
| `GUI_SB_H` | 20 | status bar height |
| `GUI_PROGRESS_H` | 14 | `gui_progress_bar` height |
| `GUI_CHECK_SZ` | 14 | `gui_checkbox` box side |
| `GUI_TOGGLE_W` | 34 | `gui_toggle` width |
| `GUI_TOGGLE_H` | 14 | `gui_toggle` height |
| `screen_char_h() + 8` | 24 | `gui_input_field` height |

## All widget signatures

```c
/* Infrastructure */
void screen_enter_vbe(void);
void screen_enter_vga(void);
int  screen_is_vbe(void);

/* Background */
void gui_desktop(void);

/* Containers */
void gui_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *title);
void gui_panel (uint32_t x, uint32_t y, uint32_t w, uint32_t h);

/* Drawing primitives */
void screen_fill_rect        (uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void screen_fill_rounded_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r, uint32_t color);
void screen_fill_gradient_h  (uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t left,  uint32_t right);
void screen_fill_gradient_v  (uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t top,   uint32_t bot);
void screen_draw_str_px      (uint32_t x, uint32_t y, const char *s, uint32_t fg, uint32_t bg);

/* Standard widgets */
void gui_button        (uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *label);
void gui_label         (uint32_t x, uint32_t y, const char *text, uint32_t fg, uint32_t bg);
void gui_label_centered(uint32_t x, uint32_t y, uint32_t w, const char *text, uint32_t fg, uint32_t bg);
void gui_separator     (uint32_t x, uint32_t y, uint32_t w);
void gui_statusbar     (const char *left, const char *right);
void gui_badge         (uint32_t x, uint32_t y, const char *text, uint32_t fg, uint32_t bg);

/* New controls */
void gui_progress_bar(uint32_t x, uint32_t y, uint32_t w, uint32_t pct, uint32_t color);
void gui_checkbox    (uint32_t x, uint32_t y, const char *label, int checked);
void gui_toggle      (uint32_t x, uint32_t y, int on);
void gui_input_field (uint32_t x, uint32_t y, uint32_t w, const char *text, int focused);
```

## Theme colours

```c
GC_DESKTOP_TOP   /* dark navy    — desktop gradient top    */
GC_DESKTOP_BOT   /* near-black   — desktop gradient bottom */
GC_WIN_BG        /* window body background                  */
GC_WIN_TITLE     /* teal         — title bar                */
GC_WIN_BORDER    /* grey         — borders and tracks       */
GC_BTN           /* dark         — button body              */
GC_BTN_BORDER    /* cyan         — button border            */
GC_TEXT          /* near-white   — primary text             */
GC_TEXT_DIM      /* mid-grey     — secondary / hint text    */
GC_SEP           /* grey         — separator lines          */
GC_STATUSBAR     /* very dark    — status bar background    */
GC_ACCENT        /* cyan         — focus, cursor, active    */
GC_DANGER        /* red          — errors, critical         */
GC_SUCCESS       /* green        — ok, on, complete         */
GC_WARN          /* yellow       — caution                  */
TRANSPARENT      /* skip bg px   — text over gradients      */
```
