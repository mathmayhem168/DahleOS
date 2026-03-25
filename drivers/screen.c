/* =============================================================
   drivers/screen.c  —  Linear framebuffer driver
   =============================================================
   Text layer:  8x8 bitmap font rendered at 2x vertical scale
                → 8x16 px character cells.
                800x600 gives 100 columns x 37 rows.

   Pixel layer: screen_fill_rect, screen_fill_gradient_h/v,
                screen_fill_rounded_rect, screen_draw_char_px …
                draw anything at any pixel coordinate.
                Used by drivers/gui.c for the widget layer.
   ============================================================= */

#include "screen.h"
#include "font.h"
#include "../libc/string.h"

/* ── Framebuffer state ──────────────────────────────────────── */
static volatile uint8_t *fb;
static uint32_t fb_pitch;
static uint32_t fb_w, fb_h;

/* Character cell: font is 8x8, rendered 2x tall = 8x16 px */
#define CW  8u
#define CH  16u

static uint32_t cur_col, cur_row;
static uint32_t cur_fg, cur_bg;

/* ── Low-level pixel write ──────────────────────────────────── */
static inline void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_w || y >= fb_h) return;
    *(uint32_t *)(fb + y * fb_pitch + x * 4) = color;
}

/* ── Pixel drawing API ──────────────────────────────────────── */

void screen_fill_rect(uint32_t x, uint32_t y,
                      uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t ry = y; ry < y + h && ry < fb_h; ry++) {
        uint32_t *row = (uint32_t *)(fb + ry * fb_pitch);
        for (uint32_t rx = x; rx < x + w && rx < fb_w; rx++)
            row[rx] = color;
    }
}

void screen_fill_gradient_h(uint32_t x, uint32_t y,
                             uint32_t w, uint32_t h,
                             uint32_t col_l, uint32_t col_r) {
    uint32_t rl = (col_l >> 16) & 0xFF, gl = (col_l >> 8) & 0xFF, bl = col_l & 0xFF;
    uint32_t rr = (col_r >> 16) & 0xFF, gr = (col_r >> 8) & 0xFF, br = col_r & 0xFF;
    uint32_t denom = (w > 1u) ? w - 1u : 1u;
    for (uint32_t cx = 0; cx < w && (x + cx) < fb_w; cx++) {
        uint32_t t = cx * 255u / denom;
        uint32_t r = (rl * (255u - t) + rr * t) / 255u;
        uint32_t g = (gl * (255u - t) + gr * t) / 255u;
        uint32_t b = (bl * (255u - t) + br * t) / 255u;
        uint32_t color = (r << 16) | (g << 8) | b;
        for (uint32_t ry = y; ry < y + h && ry < fb_h; ry++)
            put_pixel(x + cx, ry, color);
    }
}

void screen_fill_gradient_v(uint32_t x, uint32_t y,
                             uint32_t w, uint32_t h,
                             uint32_t col_top, uint32_t col_bot) {
    uint32_t rt = (col_top >> 16) & 0xFF, gt = (col_top >> 8) & 0xFF, bt = col_top & 0xFF;
    uint32_t rb = (col_bot >> 16) & 0xFF, gb = (col_bot >> 8) & 0xFF, bb = col_bot & 0xFF;
    uint32_t denom = (h > 1u) ? h - 1u : 1u;
    for (uint32_t cy = 0; cy < h && (y + cy) < fb_h; cy++) {
        uint32_t t = cy * 255u / denom;
        uint32_t r = (rt * (255u - t) + rb * t) / 255u;
        uint32_t g = (gt * (255u - t) + gb * t) / 255u;
        uint32_t b = (bt * (255u - t) + bb * t) / 255u;
        uint32_t color = (r << 16) | (g << 8) | b;
        uint32_t *row = (uint32_t *)(fb + (y + cy) * fb_pitch);
        for (uint32_t cx = x; cx < x + w && cx < fb_w; cx++)
            row[cx] = color;
    }
}

void screen_fill_rounded_rect(uint32_t x, uint32_t y,
                               uint32_t w, uint32_t h,
                               uint32_t r, uint32_t color) {
    if (r > w / 2u) r = w / 2u;
    if (r > h / 2u) r = h / 2u;
    /* Three non-corner strips */
    screen_fill_rect(x + r,     y,     w - 2u*r, h,         color);
    screen_fill_rect(x,         y + r, r,         h - 2u*r,  color);
    screen_fill_rect(x + w - r, y + r, r,         h - 2u*r,  color);
    /* Four rounded corners using a filled quarter-disc */
    for (uint32_t dy = 0; dy < r; dy++) {
        for (uint32_t dx = 0; dx < r; dx++) {
            int ipx = (int)r - (int)dx - 1;
            int ipy = (int)r - (int)dy - 1;
            if (ipx * ipx + ipy * ipy <= (int)(r * r)) {
                put_pixel(x         + dx, y         + dy, color);
                put_pixel(x + w-1u - dx, y         + dy, color);
                put_pixel(x         + dx, y + h-1u - dy, color);
                put_pixel(x + w-1u - dx, y + h-1u - dy, color);
            }
        }
    }
}

void screen_draw_char_px(uint32_t px, uint32_t py, char c,
                          uint32_t fg, uint32_t bg) {
    unsigned char uc = (unsigned char)c;
    if (uc < FONT_FIRST || uc > FONT_LAST) uc = '?';
    const uint8_t *glyph = font8x8[uc - FONT_FIRST];
    int transp = (bg == TRANSPARENT);
    for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for (uint32_t bit = 0; bit < FONT_W; bit++) {
            if (bits & (1u << bit)) {   /* LSB = leftmost pixel in this font */
                put_pixel(px + bit, py + row * 2u,      fg);
                put_pixel(px + bit, py + row * 2u + 1u, fg);
            } else if (!transp) {
                put_pixel(px + bit, py + row * 2u,      bg);
                put_pixel(px + bit, py + row * 2u + 1u, bg);
            }
        }
    }
}

void screen_draw_str_px(uint32_t x, uint32_t y, const char *s,
                         uint32_t fg, uint32_t bg) {
    for (uint32_t i = 0; s[i]; i++)
        screen_draw_char_px(x + i * CW, y, s[i], fg, bg);
}

void screen_noise(uint32_t seed) {
    for (uint32_t py = 0; py < fb_h; py += 4u) {
        for (uint32_t px = 0; px < fb_w; px += 4u) {
            seed = seed * 1664525u + 1013904223u;
            screen_fill_rect(px, py, 4u, 4u, seed & 0xFFFFFFu);
        }
    }
}

uint32_t screen_char_w(void) { return CW; }
uint32_t screen_char_h(void) { return CH; }
uint32_t screen_px_w(void)   { return fb_w; }
uint32_t screen_px_h(void)   { return fb_h; }

/* ── Text-layer internals ───────────────────────────────────── */

static uint32_t tcols(void) { return fb_w / CW; }
static uint32_t trows(void) { return fb_h / CH; }

static void do_scroll(void) {
    uint8_t *dst       = (uint8_t *)fb;
    const uint8_t *src = (const uint8_t *)fb + CH * fb_pitch;
    uint32_t bytes     = (fb_h - CH) * fb_pitch;
    for (uint32_t i = 0; i < bytes; i++) dst[i] = src[i];
    screen_fill_rect(0, fb_h - CH, fb_w, CH, cur_bg);
    if (cur_row > 0) cur_row--;
}

/* ── Public text API ────────────────────────────────────────── */

void screen_init(void *fb_addr, uint32_t pitch,
                 uint32_t width, uint32_t height, uint32_t bpp) {
    (void)bpp;
    fb       = (volatile uint8_t *)fb_addr;
    fb_pitch = pitch;
    fb_w     = width;
    fb_h     = height;
    cur_fg   = WHITE;
    cur_bg   = GUI_DESKTOP;
    cur_col  = cur_row = 0;
    screen_clear();
}

void screen_clear(void) {
    screen_fill_rect(0, 0, fb_w, fb_h, cur_bg);
    cur_col = cur_row = 0;
}

void screen_set_color(uint32_t fg, uint32_t bg) {
    cur_fg = fg;
    cur_bg = bg;
}

void kprint_char(char c) {
    uint32_t cols = tcols(), rows = trows();
    switch (c) {
    case '\n': cur_col = 0; cur_row++; break;
    case '\r': cur_col = 0; break;
    case '\t':
        cur_col = (cur_col + 8u) & ~7u;
        if (cur_col >= cols) { cur_col = 0; cur_row++; }
        break;
    default:
        screen_draw_char_px(cur_col * CW, cur_row * CH, c, cur_fg, cur_bg);
        if (++cur_col >= cols) { cur_col = 0; cur_row++; }
        break;
    }
    if (cur_row >= rows) do_scroll();
}

void kprint(const char *s) { while (*s) kprint_char(*s++); }

void kprint_color(const char *s, uint32_t fg, uint32_t bg) {
    uint32_t of = cur_fg, ob = cur_bg;
    cur_fg = fg; cur_bg = bg;
    kprint(s);
    cur_fg = of; cur_bg = ob;
}

void kprint_int(int n) {
    char buf[12]; int_to_str(n, buf); kprint(buf);
}

void kprint_color_int(int n, uint32_t fg, uint32_t bg) {
    char buf[12]; int_to_str(n, buf); kprint_color(buf, fg, bg);
}

void kprint_hex(uint32_t n) {
    char buf[11]; uint_to_hex(n, buf); kprint(buf);
}

void screen_backspace(void) {
    if (cur_col > 0) {
        cur_col--;
    } else if (cur_row > 0) {
        cur_row--;
        cur_col = tcols() - 1u;
    }
    screen_fill_rect(cur_col * CW, cur_row * CH, CW, CH, cur_bg);
}
