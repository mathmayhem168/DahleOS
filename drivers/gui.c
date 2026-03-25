/* =============================================================
   drivers/gui.c  —  High-level widget layer
   =============================================================
   Builds on the screen.c pixel API.  Callers only need to
   include gui.h; they never touch put_pixel or raw coordinates.

   Window anatomy (y offsets from window top-left):
     y + 0              window border top
     y + 1              title bar start
     y + 1 + GUI_TITLE_H separator line
     y + 2 + GUI_TITLE_H body content starts
     y + h - 1          window border bottom
   ============================================================= */

#include "gui.h"
#include "../libc/string.h"

/* Internal layout constants */
#define WIN_R   6u   /* window corner radius                    */
#define BTN_R   4u   /* button corner radius                    */
#define SB_H   20u   /* status bar height in pixels             */

/* ── Helpers ────────────────────────────────────────────────── */

/* A rectangle whose top two corners are rounded and bottom is straight.
   Composed from a full rounded rect plus a fill rect that squares the bottom. */
static void fill_top_rounded(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                              uint32_t r, uint32_t color) {
    screen_fill_rounded_rect(x, y, w, h, r, color);
    /* Fill the area carved by the bottom-corner arcs */
    if (h > r)
        screen_fill_rect(x, y + h - r, w, r, color);
}

/* ── Widget implementations ─────────────────────────────────── */

void gui_desktop(void) {
    screen_fill_gradient_v(0, 0, screen_px_w(), screen_px_h(),
                           GC_DESKTOP_TOP, GC_DESKTOP_BOT);
}

void gui_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                const char *title) {
    /* Drop shadow */
    screen_fill_rounded_rect(x + 4, y + 5, w, h, WIN_R, RGB(0, 0, 0));

    /* Window border (1-px outline) */
    screen_fill_rounded_rect(x, y, w, h, WIN_R, GC_WIN_BORDER);

    /* Window body (inset 1 px) */
    screen_fill_rounded_rect(x + 1, y + 1, w - 2, h - 2, WIN_R - 1, GC_WIN_BG);

    /* Title bar: top corners rounded, bottom edge straight */
    fill_top_rounded(x + 1, y + 1, w - 2, GUI_TITLE_H, WIN_R - 1, GC_WIN_TITLE);

    /* Separator line below title bar */
    screen_fill_rect(x + 1, y + 1 + GUI_TITLE_H, w - 2, 1, GC_WIN_BORDER);

    /* Title text centred in the title bar */
    uint32_t ty = y + 1 + (GUI_TITLE_H - screen_char_h()) / 2;
    gui_label_centered(x + 1, ty, w - 2, title, GC_TEXT, GC_WIN_TITLE);
}

void gui_panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    /* Border */
    screen_fill_rounded_rect(x, y, w, h, WIN_R, GC_WIN_BORDER);
    /* Body */
    screen_fill_rounded_rect(x + 1, y + 1, w - 2, h - 2, WIN_R - 1, GC_WIN_BG);
}

void gui_button(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                const char *label) {
    /* Accent border */
    screen_fill_rounded_rect(x, y, w, h, BTN_R, GC_BTN_BORDER);
    /* Button body (inset 1 px) */
    screen_fill_rounded_rect(x + 1, y + 1, w - 2, h - 2, BTN_R - 1, GC_BTN);
    /* Centred label */
    uint32_t ty = y + 1 + (h - 2 > screen_char_h() ? (h - 2 - screen_char_h()) / 2 : 0);
    gui_label_centered(x + 1, ty, w - 2, label, GC_TEXT, GC_BTN);
}

void gui_label(uint32_t x, uint32_t y,
               const char *text, uint32_t fg, uint32_t bg) {
    screen_draw_str_px(x, y, text, fg, bg);
}

void gui_label_centered(uint32_t x, uint32_t y, uint32_t w,
                        const char *text, uint32_t fg, uint32_t bg) {
    uint32_t tw = (uint32_t)strlen(text) * screen_char_w();
    uint32_t tx = x + (tw < w ? (w - tw) / 2u : 0u);
    screen_draw_str_px(tx, y, text, fg, bg);
}

void gui_separator(uint32_t x, uint32_t y, uint32_t w) {
    screen_fill_rect(x, y, w, 1, GC_SEP);
}

void gui_statusbar(const char *left_text, const char *right_text) {
    uint32_t sw = screen_px_w();
    uint32_t sy = screen_px_h() - SB_H;

    /* Background */
    screen_fill_rect(0, sy, sw, SB_H, GC_STATUSBAR);
    /* Top border */
    screen_fill_rect(0, sy, sw, 1, GC_WIN_BORDER);

    uint32_t ty = sy + (SB_H - screen_char_h()) / 2;

    if (left_text)
        screen_draw_str_px(GUI_PAD, ty, left_text, GC_TEXT, GC_STATUSBAR);

    if (right_text) {
        uint32_t rtw = (uint32_t)strlen(right_text) * screen_char_w();
        screen_draw_str_px(sw - rtw - GUI_PAD, ty,
                           right_text, GC_TEXT_DIM, GC_STATUSBAR);
    }
}

void gui_badge(uint32_t x, uint32_t y,
               const char *text, uint32_t fg, uint32_t bg) {
    uint32_t tw = (uint32_t)strlen(text) * screen_char_w();
    uint32_t bw = tw + 10u;
    uint32_t bh = screen_char_h() + 6u;
    screen_fill_rounded_rect(x, y, bw, bh, 3, bg);
    screen_draw_str_px(x + 5, y + 3, text, fg, bg);
}
