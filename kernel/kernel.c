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
#include "../drivers/vbe.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../shell/shell.h"

/* ── Kernel entry ───────────────────────────────────────────── */
void kmain(uint32_t *mb_ptr) {
    (void)mb_ptr;

    /* Probe for the Bochs VGA card and save its framebuffer address for later
       use by screen_enter_vbe() (called by the GUI or 'graphics set vbe').
       VBE mode is NOT activated here — the kernel always boots in VGA text
       mode so the shell is ready immediately after hardware init. */
    vbe_init();

    /* Boot in VGA hardware text mode (0xB8000). */
    screen_enter_vga();

    /* Boot banner */
    kprint_color("\n  " OS_NAME "  v" OS_VERSION "\n", LGREEN, BLACK);
    kprint_color("  ----------------\n\n", LGREY, BLACK);

    /* Hardware init */
    gdt_install();
    idt_install();
    isr_install();
    irq_install();
    timer_install(100);
    keyboard_install();
    __asm__ volatile("sti");

    kprint_color("  All systems nominal.\n\n", GREEN, BLACK);

    shell_run();

    for (;;) __asm__ volatile("hlt");
}
