#ifndef DRIVERS_FONT_H
#define DRIVERS_FONT_H
#include "../include/types.h"

/* 8x8 bitmap font covering ASCII 0x20 (space) through 0x7E (~).
 * Each character = 8 bytes, one byte per row.
 * Bit 7 of each byte is the leftmost pixel; bit 0 is the rightmost. */

#define FONT_W          8
#define FONT_HEIGHT     8
#define FONT_FIRST      0x20
#define FONT_LAST       0x7E
#define FONT_COUNT      (FONT_LAST - FONT_FIRST + 1)  /* 95 */

extern const uint8_t font8x8[FONT_COUNT][FONT_HEIGHT];

#endif
