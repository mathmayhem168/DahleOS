#include "isr.h"
#include "idt.h"
#include "../drivers/screen.h"
#include "../libc/mem.h"

isr_t interrupt_handlers[256];

/* Forward-declare all 32 exception stubs (defined in isr.asm) */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

void isr_install(void) {
    memset(interrupt_handlers, 0, sizeof(interrupt_handlers));
#define G(n) idt_set_gate(n, (uint32_t)isr##n, 0x08, 0x8E)
    G(0);  G(1);  G(2);  G(3);  G(4);  G(5);  G(6);  G(7);
    G(8);  G(9);  G(10); G(11); G(12); G(13); G(14); G(15);
    G(16); G(17); G(18); G(19); G(20); G(21); G(22); G(23);
    G(24); G(25); G(26); G(27); G(28); G(29); G(30); G(31);
#undef G
}

void register_interrupt_handler(uint8_t n, isr_t fn) {
    interrupt_handlers[n] = fn;
}

/* Called from isr.asm */
static const char *exc_name[32] = {
    "Divide-by-Zero",       "Debug",              "NMI",
    "Breakpoint",           "Overflow",           "Bound Range",
    "Invalid Opcode",       "No FPU",             "Double Fault",
    "FPU Seg Overrun",      "Invalid TSS",        "Segment Not Present",
    "Stack Fault",          "General Protection", "Page Fault",
    "Reserved",             "x87 FP Error",       "Alignment Check",
    "Machine Check",        "SIMD FP",            "Reserved",
    "Reserved","Reserved","Reserved","Reserved","Reserved",
    "Reserved","Reserved","Reserved","Reserved",
    "Security Exception",   "Reserved"
};

void isr_handler(registers_t *r) {
    if (interrupt_handlers[r->int_no]) {
        interrupt_handlers[r->int_no](r);
        return;
    }
    /* Unhandled exception → kernel panic */
    screen_set_color(WHITE, RED);
    kprint("\n\n  *** KERNEL PANIC ***\n");
    kprint("  Exception #"); kprint_int((int)r->int_no);
    kprint(" – ");
    if (r->int_no < 32) kprint(exc_name[r->int_no]);
    kprint("\n  EIP="); kprint_hex(r->eip);
    kprint("  ERR="); kprint_hex(r->err_code);
    kprint("\n\n  System halted.\n");
    for (;;) __asm__ volatile("hlt");
}
