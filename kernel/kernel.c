/* =============================================================
   kernel/kernel.c  —  Entry point after boot.asm
   =============================================================
   Reads the Multiboot info struct to obtain the linear
   framebuffer address, initialises all hardware subsystems,
   then hands control to the interactive shell.
   ============================================================= */

#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../cpu/irq.h"
#include "../cpu/timer.h"
#include "../drivers/port.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../shell/shell.h"

/* ── Multiboot info structure (spec section 3.3) ────────────── */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower, mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count, mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length, mmap_addr;
    uint32_t drives_length, drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    /* Framebuffer — present when flags bit 12 is set */
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;   /* 1 = direct RGB colour */
    uint8_t  color_info[6];
} __attribute__((packed)) mb_info_t;

#define MB_FLAG_FB  (1u << 12)

/* ── Bochs VGA / VBE helpers ────────────────────────────────── */

/* Read 32 bits from PCI configuration space (bus 0 only). */
static uint32_t pci_read32(uint8_t dev, uint8_t off) {
    port_long_out(0xCF8u,
        0x80000000u | ((uint32_t)dev << 11) | (off & 0xFCu));
    return port_long_in(0xCFCu);
}

/* Write to a Bochs VBE register (ports 0x01CE / 0x01CF). */
static void vbe_write(uint16_t idx, uint16_t val) {
    port_word_out(0x01CEu, idx);
    port_word_out(0x01CFu, val);
}

/* Scan PCI bus 0 for the Bochs VGA card (vendor 0x1234, device 0x1111).
   If found, program it for 800x600x32 linear-framebuffer mode and
   return the framebuffer base address from BAR 0.
   Returns 0 when no Bochs VGA is present. */
static uint32_t bochs_vga_init(void) {
    for (uint8_t dev = 0; dev < 32u; dev++) {
        /* PCI word 0 = (device_id << 16) | vendor_id */
        if (pci_read32(dev, 0) != 0x11111234u)
            continue;

        /* Found — read the linear framebuffer address from BAR 0. */
        uint32_t bar0 = pci_read32(dev, 0x10) & 0xFFFFFFF0u;

        /* Program the Bochs VBE interface.
           Always reset virtual size and viewport offsets so the
           hardware pitch matches XRES*4 and scan starts at (0,0). */
        vbe_write(4, 0);        /* disable while reconfiguring          */
        vbe_write(1, 800);      /* horizontal resolution                */
        vbe_write(2, 600);      /* vertical resolution                  */
        vbe_write(3, 32);       /* bits per pixel                       */
        vbe_write(6, 800);      /* virtual width  = XRES (pitch=XRES*4) */
        vbe_write(7, 600);      /* virtual height = YRES                */
        vbe_write(8, 0);        /* X offset = 0                         */
        vbe_write(9, 0);        /* Y offset = 0                         */
        vbe_write(4, 0x41u);    /* enable (bit 0) + linear FB (bit 6)  */

        return bar0;
    }
    return 0;
}

/* ── Kernel entry ───────────────────────────────────────────── */
void kmain(uint32_t *mb_ptr) {
    mb_info_t *mb = (mb_info_t *)mb_ptr;

    /* 1. Try Bochs VGA (-vga std).  Works on QEMU 9/10 where Cirrus
          is deprecated and 0xFD000000 no longer maps to video RAM. */
    uint32_t bochs_fb = bochs_vga_init();
    if (bochs_fb) {
        screen_init((void *)bochs_fb, 800u * 4u, 800u, 600u, 32u);
    }
    /* 2. Use the framebuffer address reported by the Multiboot loader
          (works when GRUB or a recent QEMU sets up VBE for us). */
    else if (mb && (mb->flags & MB_FLAG_FB) && mb->framebuffer_type == 1) {
        screen_init(
            (void *)(uint32_t)mb->framebuffer_addr,
            mb->framebuffer_pitch,
            mb->framebuffer_width,
            mb->framebuffer_height,
            mb->framebuffer_bpp);
    }
    /* 3. Legacy fallback — Cirrus GD5446 linear FB on older QEMU. */
    else {
        screen_init((void *)0xFD000000u, 800u * 4u, 800u, 600u, 32u);
    }

    /* Boot banner */
    kprint_color("\n  DahleOS  v1.0.0\n", LGREEN, GUI_DESKTOP);
    kprint_color("  ----------------\n\n", GUI_BORDER, GUI_DESKTOP);

    /* Hardware init */
    gdt_install();
    idt_install();
    isr_install();
    irq_install();
    timer_install(100);
    keyboard_install();
    __asm__ volatile("sti");

    kprint_color("  All systems nominal.\n\n", GREEN, GUI_DESKTOP);

    shell_run();

    for (;;) __asm__ volatile("hlt");
}
