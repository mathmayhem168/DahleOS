#ifndef DRIVERS_VBE_H
#define DRIVERS_VBE_H
#include "../include/types.h"

/* Scan PCI bus 0 for the Bochs VGA card (vendor 0x1234 / device 0x1111).
   Saves the linear framebuffer base address (BAR 0) internally, programs
   the card for 800×600×32 VBE mode, and returns the framebuffer address.
   Returns 0 if no Bochs VGA card is found. */
uint32_t vbe_init(void);

/* Re-program the Bochs VBE interface for 800×600×32 linear-framebuffer
   mode.  Call after vbe_init() has located the card. */
void vbe_enable(void);

/* Disable VBE by clearing the enable register.  The card reverts to
   standard VGA emulation (text mode via 0xB8000). */
void vbe_disable(void);

/* Return the framebuffer base address saved during vbe_init().
   Returns 0 if vbe_init() was never called or found no card. */
uint32_t vbe_framebuffer(void);

/* Return the row pitch in bytes as read back from the hardware after the
   last vbe_enable() call.  The card may round the virtual width up for
   alignment, so always use this value rather than width * 4. */
uint32_t vbe_pitch(void);

#endif
