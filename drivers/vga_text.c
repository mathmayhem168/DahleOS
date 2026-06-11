/* =============================================================
   drivers/vga_text.c  —  VGA hardware text mode (mode 3h)
   =============================================================
   Writes directly to the VGA text buffer at 0xB8000.
   Active when VBE is disabled: Bochs VBE card reverts to
   standard VGA emulation and renders from 0xB8000.

   Character cells: 2 bytes each — low byte = ASCII code,
                                    high byte = attribute.
   Attribute:   bits 7:4 = background colour (4-bit VGA index)
                bits 3:0 = foreground colour (4-bit VGA index)
   ============================================================= */

#include "vga_text.h"
#include "font.h"
#include "port.h"

#define VGA_BUF  ((volatile uint16_t *)0xB8000u)

/* CRT controller ports */
#define CRT_IDX  0x3D4u
#define CRT_DAT  0x3D5u

/* VGA Sequencer and Graphics Controller ports */
#define SEQ_IDX  0x3C4u
#define SEQ_DAT  0x3C5u
#define GC_IDX   0x3CEu
#define GC_DAT   0x3CFu

static uint32_t vga_col;
static uint32_t vga_row;
static uint8_t  vga_attr = 0x07u;   /* default: light-gray on black */

/* ── VGA font loading ───────────────────────────────────────── */

/* Register values for standard text mode (mode 3h):
     SR2 Map Mask     : 0x03  (write planes 0 and 1)
     SR4 Memory Mode  : 0x02  (extended VRAM, odd/even interleave)
     GR4 Read Map     : 0x00  (read plane 0)
     GR5 Mode         : 0x10  (odd/even host write)
     GR6 Misc         : 0x0E  (B8000 base, 32 KB, chain odd/even, alpha) */

static void vga_set_text_mode_regs(void) {
    port_byte_out(SEQ_IDX, 2u); port_byte_out(SEQ_DAT, 0x03u);
    port_byte_out(SEQ_IDX, 4u); port_byte_out(SEQ_DAT, 0x02u);
    port_byte_out(GC_IDX,  4u); port_byte_out(GC_DAT,  0x00u);
    port_byte_out(GC_IDX,  5u); port_byte_out(GC_DAT,  0x10u);
    port_byte_out(GC_IDX,  6u); port_byte_out(GC_DAT,  0x0Eu);
}

/* Switch sequencer/GC to write to plane 2 only sequentially. */
static void vga_set_plane2_write(void) {
    port_byte_out(SEQ_IDX, 2u); port_byte_out(SEQ_DAT, 0x04u); /* plane 2 only */
    port_byte_out(SEQ_IDX, 4u); port_byte_out(SEQ_DAT, 0x06u); /* sequential   */
    port_byte_out(GC_IDX,  5u); port_byte_out(GC_DAT,  0x00u); /* write mode 0 */
    port_byte_out(GC_IDX,  6u); port_byte_out(GC_DAT,  0x00u); /* A0000+128KB  */
}

/* Reverse the bits in a byte.
   Our font8x8 uses LSB = leftmost pixel; VGA hardware expects MSB = leftmost. */
static uint8_t bit_rev8(uint8_t b) {
    b = (uint8_t)(((b & 0xF0u) >> 4u) | ((b & 0x0Fu) << 4u));
    b = (uint8_t)(((b & 0xCCu) >> 2u) | ((b & 0x33u) << 2u));
    b = (uint8_t)(((b & 0xAAu) >> 1u) | ((b & 0x55u) << 1u));
    return b;
}

void vga_text_load_font(void) {
    vga_set_plane2_write();
    volatile uint8_t *win = (volatile uint8_t *)0xA0000u;

    for (uint32_t ch = 0u; ch < 256u; ch++) {
        uint32_t base = ch * 32u;   /* VGA plane 2: 32 bytes per glyph slot */

        if (ch >= (uint32_t)FONT_FIRST && ch <= (uint32_t)FONT_LAST) {
            const uint8_t *g = font8x8[ch - FONT_FIRST];
            for (uint32_t row = 0u; row < (uint32_t)FONT_HEIGHT; row++) {
                uint8_t bits = bit_rev8(g[row]);
                win[base + row * 2u]       = bits;  /* scan line 2N   */
                win[base + row * 2u + 1u]  = bits;  /* scan line 2N+1 */
            }
            /* Zero the unused upper half of the 32-byte slot */
            for (uint32_t i = (uint32_t)FONT_HEIGHT * 2u; i < 32u; i++)
                win[base + i] = 0u;
        } else {
            /* No glyph for this code point: blank */
            for (uint32_t i = 0u; i < 32u; i++)
                win[base + i] = 0u;
        }
    }

    vga_set_text_mode_regs();
}

/* ── Hardware cursor helpers ────────────────────────────────── */

static void update_cursor(void) {
    uint16_t pos = (uint16_t)(vga_row * VGA_TEXT_COLS + vga_col);
    port_byte_out(CRT_IDX, 0x0Fu); port_byte_out(CRT_DAT, (uint8_t)(pos & 0xFFu));
    port_byte_out(CRT_IDX, 0x0Eu); port_byte_out(CRT_DAT, (uint8_t)(pos >> 8));
}

/* Enable the underline cursor (scan lines 14–15 of a 16-line cell). */
static void show_cursor(void) {
    port_byte_out(CRT_IDX, 0x0Au); port_byte_out(CRT_DAT, 14u);
    port_byte_out(CRT_IDX, 0x0Bu); port_byte_out(CRT_DAT, 15u);
}

/* ── Scroll ─────────────────────────────────────────────────── */

static void do_scroll(void) {
    volatile uint16_t *buf = VGA_BUF;
    uint32_t i;
    for (i = 0; i < (VGA_TEXT_ROWS - 1u) * VGA_TEXT_COLS; i++)
        buf[i] = buf[i + VGA_TEXT_COLS];
    uint16_t blank = (uint16_t)((uint16_t)vga_attr << 8) | (uint16_t)' ';
    for (i = (VGA_TEXT_ROWS - 1u) * VGA_TEXT_COLS;
         i < VGA_TEXT_ROWS * VGA_TEXT_COLS; i++)
        buf[i] = blank;
    if (vga_row > 0u) vga_row--;
}

/* ── Public API ─────────────────────────────────────────────── */

void vga_text_clear(void) {
    volatile uint16_t *buf = VGA_BUF;
    uint16_t blank = (uint16_t)((uint16_t)vga_attr << 8) | (uint16_t)' ';
    for (uint32_t i = 0; i < VGA_TEXT_ROWS * VGA_TEXT_COLS; i++)
        buf[i] = blank;
    vga_col = vga_row = 0;
    update_cursor();
}

void vga_text_init(void) {
    /* Explicitly restore the sequencer and graphics controller to text mode.
       Bochs VBE may leave these in a graphics state after vbe_disable(). */
    vga_set_text_mode_regs();
    vga_attr = 0x07u;   /* light-gray on black */
    show_cursor();
    vga_text_clear();
}

void vga_text_set_attr(uint8_t attr) { vga_attr = attr; }

void vga_text_putchar(char c) {
    volatile uint16_t *buf = VGA_BUF;

    switch (c) {
    case '\n': vga_col = 0; vga_row++; break;
    case '\r': vga_col = 0; break;
    case '\t':
        vga_col = (vga_col + 8u) & ~7u;
        if (vga_col >= VGA_TEXT_COLS) { vga_col = 0; vga_row++; }
        break;
    default:
        buf[vga_row * VGA_TEXT_COLS + vga_col] =
            (uint16_t)((uint16_t)vga_attr << 8) | (uint8_t)c;
        if (++vga_col >= VGA_TEXT_COLS) { vga_col = 0; vga_row++; }
        break;
    }
    if (vga_row >= VGA_TEXT_ROWS) do_scroll();
    update_cursor();
}

void vga_text_print(const char *s) {
    while (*s) vga_text_putchar(*s++);
}

void vga_text_backspace(void) {
    volatile uint16_t *buf = VGA_BUF;
    if (vga_col > 0u) {
        vga_col--;
    } else if (vga_row > 0u) {
        vga_row--;
        vga_col = VGA_TEXT_COLS - 1u;
    }
    buf[vga_row * VGA_TEXT_COLS + vga_col] =
        (uint16_t)((uint16_t)vga_attr << 8) | (uint16_t)' ';
    update_cursor();
}

/* ── RGB → VGA palette mapping ──────────────────────────────── */

/* Map the named colours from screen.h to their nearest VGA 4-bit index. */
static uint8_t rgb_to_vga4(uint32_t rgb) {
    switch (rgb) {
    case 0x0D1117u: return 0x0u;   /* BLACK / GUI_DESKTOP  → black       */
    case 0x388BFDu: return 0x9u;   /* BLUE  / GUI_ACCENT   → lt. blue    */
    case 0x3FB950u: return 0x2u;   /* GREEN                → green       */
    case 0x39C5CFu: return 0x3u;   /* CYAN                 → cyan        */
    case 0xF85149u: return 0x4u;   /* RED                  → red         */
    case 0xBC8CFFu: return 0xDu;   /* MAGENTA              → lt. magenta */
    case 0xD29922u: return 0x6u;   /* BROWN                → brown       */
    case 0x8B949Eu: return 0x7u;   /* LGREY                → lt. gray    */
    case 0x21262Du: return 0x8u;   /* DGREY                → dk. gray    */
    case 0x79C0FFu: return 0x1u;   /* LBLUE                → blue        */
    case 0x56D364u: return 0xAu;   /* LGREEN               → lt. green   */
    case 0x56D4DDu: return 0xBu;   /* LCYAN                → lt. cyan    */
    case 0xFF7B72u: return 0xCu;   /* LRED                 → lt. red     */
    case 0xD2A8FFu: return 0x5u;   /* LMAGENTA             → magenta     */
    case 0xE3B341u: return 0xEu;   /* LBROWN               → yellow      */
    case 0xF0F6FCu: return 0xFu;   /* WHITE                → white       */
    case 0x161B22u: return 0x0u;   /* GUI_PANEL            → black       */
    case 0x30363Du: return 0x8u;   /* GUI_BORDER           → dk. gray    */
    case 0x1C2128u: return 0x0u;   /* GUI_WIN_BG           → black       */
    default:        return 0x7u;   /* fallback             → lt. gray    */
    }
}

uint8_t vga_text_rgb_to_attr(uint32_t fg, uint32_t bg) {
    return (uint8_t)((rgb_to_vga4(bg) << 4) | rgb_to_vga4(fg));
}
