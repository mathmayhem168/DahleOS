#ifndef DRIVERS_SCREEN_H
#define DRIVERS_SCREEN_H
#include "../include/types.h"

/* =============================================================
   Colour palette — 24-bit RGB values for the linear framebuffer.
   Classic VGA names kept for familiarity.
   ============================================================= */
#define BLACK       0x0D1117u
#define BLUE        0x388BFDu
#define GREEN       0x3FB950u
#define CYAN        0x39C5CFu
#define RED         0xF85149u
#define MAGENTA     0xBC8CFFu
#define BROWN       0xD29922u
#define LGREY       0x8B949Eu
#define DGREY       0x21262Du
#define LBLUE       0x79C0FFu
#define LGREEN      0x56D364u
#define LCYAN       0x56D4DDu
#define LRED        0xFF7B72u
#define LMAGENTA    0xD2A8FFu
#define LBROWN      0xE3B341u
#define WHITE       0xF0F6FCu

/* GUI-specific aliases */
#define GUI_DESKTOP  0x0D1117u
#define GUI_PANEL    0x161B22u
#define GUI_BORDER   0x30363Du
#define GUI_ACCENT   0x388BFDu
#define GUI_WIN_BG   0x1C2128u

/* Build a 24-bit colour from components */
#define RGB(r,g,b) \
    ((uint32_t)(((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))

/* Pass as the `bg` argument to skip drawing background pixels,
   useful when drawing text on top of gradients or images. */
#define TRANSPARENT  0xFF000000u

/* =============================================================
   Initialisation
   ============================================================= */
void screen_init(void *fb_addr, uint32_t pitch,
                 uint32_t width, uint32_t height, uint32_t bpp);

/* =============================================================
   Text API  (cursor-based, wraps and scrolls automatically)
   ============================================================= */
void screen_clear(void);
void screen_set_color(uint32_t fg, uint32_t bg);
void kprint(const char *s);
void kprint_char(char c);
void kprint_color(const char *s, uint32_t fg, uint32_t bg);
void kprint_color_int(int n, uint32_t fg, uint32_t bg);
void kprint_int(int n);
void kprint_hex(uint32_t n);
void screen_backspace(void);

/* =============================================================
   Pixel drawing API  (all coordinates/dimensions in pixels)
   ============================================================= */
void screen_fill_rect(uint32_t x, uint32_t y,
                      uint32_t w, uint32_t h, uint32_t color);

void screen_fill_gradient_h(uint32_t x, uint32_t y,
                             uint32_t w, uint32_t h,
                             uint32_t col_l, uint32_t col_r);

void screen_fill_gradient_v(uint32_t x, uint32_t y,
                             uint32_t w, uint32_t h,
                             uint32_t col_top, uint32_t col_bot);

void screen_fill_rounded_rect(uint32_t x, uint32_t y,
                               uint32_t w, uint32_t h,
                               uint32_t radius, uint32_t color);

/* Draw a single bitmap character at pixel coords.
   Pass TRANSPARENT as `bg` to skip background pixels. */
void screen_draw_char_px(uint32_t x, uint32_t y, char c,
                          uint32_t fg, uint32_t bg);

/* Draw a NUL-terminated string at pixel coords. */
void screen_draw_str_px(uint32_t x, uint32_t y, const char *s,
                         uint32_t fg, uint32_t bg);

void screen_noise(uint32_t seed);

/* Dimensions */
uint32_t screen_char_w(void);   /* character cell width  in pixels (8)  */
uint32_t screen_char_h(void);   /* character cell height in pixels (16) */
uint32_t screen_px_w(void);     /* framebuffer width  in pixels         */
uint32_t screen_px_h(void);     /* framebuffer height in pixels         */

#endif
