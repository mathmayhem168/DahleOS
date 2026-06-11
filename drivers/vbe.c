/* =============================================================
   drivers/vbe.c  —  Bochs VBE hardware control
   =============================================================
   Programs the Bochs VBE extension (ports 0x01CE / 0x01CF) that
   QEMU exposes when started with -vga std.  Provides the three
   runtime entry-points used by screen.c to switch between VBE
   graphics mode and standard VGA text mode.
   ============================================================= */

#include "vbe.h"
#include "port.h"

/* Bochs VBE register indices */
#define VBE_IDX  0x01CEu
#define VBE_DAT  0x01CFu

/* Saved linear framebuffer base address (PCI BAR 0) and actual row pitch. */
static uint32_t saved_bar0;
static uint32_t saved_pitch;

/* ── PCI helpers ────────────────────────────────────────────── */

static uint32_t pci_read32(uint8_t dev, uint8_t off) {
    port_long_out(0xCF8u,
        0x80000000u | ((uint32_t)dev << 11) | (uint32_t)(off & 0xFCu));
    return port_long_in(0xCFCu);
}

/* ── Public API ─────────────────────────────────────────────── */

uint32_t vbe_init(void) {
    for (uint8_t dev = 0; dev < 32u; dev++) {
        /* PCI config word 0: (device_id << 16) | vendor_id */
        if (pci_read32(dev, 0) != 0x11111234u)
            continue;

        saved_bar0 = pci_read32(dev, 0x10u) & 0xFFFFFFF0u;
        /* Do NOT enable VBE here — just record the framebuffer address.
           The VGA display controller must not be disturbed until after
           gdt_install/idt_install complete and screen_enter_vbe() is
           explicitly called (by the GUI or 'graphics set vbe'). */
        return saved_bar0;
    }
    return 0;
}

void vbe_enable(void) {
    port_word_out(VBE_IDX, 4u); port_word_out(VBE_DAT, 0u);       /* disable first  */
    port_word_out(VBE_IDX, 1u); port_word_out(VBE_DAT, 800u);     /* width          */
    port_word_out(VBE_IDX, 2u); port_word_out(VBE_DAT, 600u);     /* height         */
    port_word_out(VBE_IDX, 3u); port_word_out(VBE_DAT, 32u);      /* bpp            */
    port_word_out(VBE_IDX, 6u); port_word_out(VBE_DAT, 800u);     /* virtual width  */
    /* Enable bit (0) + linear framebuffer bit (6) */
    port_word_out(VBE_IDX, 4u); port_word_out(VBE_DAT, 0x41u);

    /* Read back the virtual width the hardware accepted — it may round up.
       Pitch = virt_width * 4 bytes per pixel. */
    port_word_out(VBE_IDX, 6u);
    uint16_t vw = port_word_in(VBE_DAT);
    saved_pitch = vw ? (uint32_t)vw * 4u : 800u * 4u;
}

void vbe_disable(void) {
    port_word_out(VBE_IDX, 4u);
    port_word_out(VBE_DAT, 0u);
}

uint32_t vbe_framebuffer(void) { return saved_bar0; }
uint32_t vbe_pitch(void)       { return saved_pitch; }
