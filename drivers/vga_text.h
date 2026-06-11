#ifndef DRIVERS_VGA_TEXT_H
#define DRIVERS_VGA_TEXT_H
#include "../include/types.h"

#define VGA_TEXT_COLS 80u
#define VGA_TEXT_ROWS 25u

/* Switch to VGA text mode: restore sequencer/GC registers, show cursor, clear. */
void vga_text_init(void);

/* Write our embedded 8×8 font (doubled to 8×16) into VGA plane 2 so that
   the hardware has a valid glyph table after returning from VBE mode.
   Also restores the sequencer and graphics-controller register state. */
void vga_text_load_font(void);

/* Fill the text buffer with spaces using the current attribute. */
void vga_text_clear(void);

/* Print a single character at the current cursor position.
   Handles '\n', '\r', '\t', and scrolls when the last row is exceeded. */
void vga_text_putchar(char c);

/* Print a NUL-terminated string. */
void vga_text_print(const char *s);

/* Erase the character to the left of the cursor (classic backspace). */
void vga_text_backspace(void);

/* Set the VGA attribute byte used for subsequent character output.
   attr = (bg_color_4bit << 4) | fg_color_4bit */
void vga_text_set_attr(uint8_t attr);

/* Convert a pair of 24-bit RGB colours (from screen.h) to a VGA
   attribute byte by mapping each to the nearest VGA palette index. */
uint8_t vga_text_rgb_to_attr(uint32_t fg, uint32_t bg);

#endif
