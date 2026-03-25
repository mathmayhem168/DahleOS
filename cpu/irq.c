#include "irq.h"
#include "isr.h"
#include "idt.h"
#include "../drivers/port.h"

extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

static void pic_remap(void) {
    /* Start init sequence */
    port_byte_out(0x20, 0x11);  port_byte_out(0xA0, 0x11);
    /* Vector offsets */
    port_byte_out(0x21, 0x20);  port_byte_out(0xA1, 0x28);
    /* Cascade wiring */
    port_byte_out(0x21, 0x04);  port_byte_out(0xA1, 0x02);
    /* 8086 mode */
    port_byte_out(0x21, 0x01);  port_byte_out(0xA1, 0x01);
    /* Unmask all */
    port_byte_out(0x21, 0x00);  port_byte_out(0xA1, 0x00);
}

void irq_install(void) {
    pic_remap();
#define G(n,v) idt_set_gate(v, (uint32_t)irq##n, 0x08, 0x8E)
    G(0,32); G(1,33); G(2,34);  G(3,35);  G(4,36);  G(5,37);
    G(6,38); G(7,39); G(8,40);  G(9,41);  G(10,42); G(11,43);
    G(12,44);G(13,45);G(14,46); G(15,47);
#undef G
}

/* Called from irq.asm */
void irq_handler(registers_t *r) {
    if (r->int_no >= 40) port_byte_out(0xA0, 0x20); /* EOI → slave  */
    port_byte_out(0x20, 0x20);                       /* EOI → master */
    if (interrupt_handlers[r->int_no])
        interrupt_handlers[r->int_no](r);
}
