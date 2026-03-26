#ifndef DRIVERS_GUI_H
#define DRIVERS_GUI_H

/* gui.h builds on screen.h — includes it so callers only need one header */
#include "screen.h"

/* Pass as `bg` to any drawing function to skip background pixels,
   e.g. text rendered over a gradient without black cell squares.
   Defined here in case screen.h was not included directly. */
#ifndef TRANSPARENT
#define TRANSPARENT  0xFF000000u
#endif

/* =============================================================
   Theme colour constants
   All values are 24-bit RGB, matching the screen.h RGB() macro.
   ============================================================= */
#define GC_DESKTOP_TOP  RGB(18,  26,  48)   /* desktop gradient – top    */
#define GC_DESKTOP_BOT  RGB(10,  14,  22)   /* desktop gradient – bottom */
#define GC_WIN_BG       RGB(22,  27,  34)   /* window body               */
#define GC_WIN_TITLE    RGB(0,  120, 140)   /* title bar (teal)          */
#define GC_WIN_BORDER   RGB(48,  54,  61)   /* window / panel border     */
#define GC_BTN          RGB(33,  39,  46)   /* button background         */
#define GC_BTN_BORDER   RGB(0,  180, 180)   /* button border (cyan)      */
#define GC_TEXT         RGB(240, 246, 252)  /* primary text              */
#define GC_TEXT_DIM     RGB(139, 148, 158)  /* secondary / dim text      */
#define GC_SEP          RGB(48,  54,  61)   /* separator line            */
#define GC_STATUSBAR    RGB(13,  17,  23)   /* status bar background     */
#define GC_ACCENT       RGB(0,  180, 180)   /* accent cyan               */
#define GC_DANGER       RGB(248,  81,  73)  /* red – errors / warnings   */
#define GC_SUCCESS      RGB( 63, 185,  80)  /* green – ok / success      */
#define GC_WARN         RGB(227, 179,  65)  /* yellow – warnings         */

/* =============================================================
   Widget functions
   All coordinates and dimensions are in pixels.
   ============================================================= */

/* Fill the entire screen with a vertical gradient (desktop background). */
void gui_desktop(void);

/* Window with a gradient title bar, rounded corners, and a drop shadow.
   The content area begins at  y + GUI_TITLE_H + 1  pixels from the top. */
void gui_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                const char *title);

/* Borderless content panel — rounded rect with a 1-px border. */
void gui_panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

/* Clickable-looking button with a rounded border and centred label. */
void gui_button(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                const char *label);

/* Plain text at (x, y).
   Pass TRANSPARENT as `bg` to render over a gradient without black squares. */
void gui_label(uint32_t x, uint32_t y,
               const char *text, uint32_t fg, uint32_t bg);

/* Same as gui_label but the text is centred inside a region of width `w`. */
void gui_label_centered(uint32_t x, uint32_t y, uint32_t w,
                        const char *text, uint32_t fg, uint32_t bg);

/* Horizontal 1-px separator line. */
void gui_separator(uint32_t x, uint32_t y, uint32_t w);

/* Full-width status bar pinned to the bottom of the screen.
   `left_text` is left-aligned; `right_text` is right-aligned.
   Either may be NULL to omit. */
void gui_statusbar(const char *left_text, const char *right_text);

/* Small coloured pill / badge — useful for tags, status chips, counts. */
void gui_badge(uint32_t x, uint32_t y,
               const char *text, uint32_t fg, uint32_t bg);

/* Height of the window title bar in pixels (useful for content placement). */
#define GUI_TITLE_H  22u

/* Padding inside a window content area. */
#define GUI_PAD      10u

#endif
