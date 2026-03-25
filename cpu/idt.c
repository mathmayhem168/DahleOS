#include "idt.h"
#include "../libc/mem.h"

#define IDT_ENTRIES 256
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;
extern void idt_flush(uint32_t);

void idt_set_gate(uint8_t n, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[n].base_lo = base & 0xFFFF;
    idt[n].base_hi = (base >> 16) & 0xFFFF;
    idt[n].sel     = sel;
    idt[n].zero    = 0;
    idt[n].flags   = flags;
}

void idt_install(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)idt;
    memset(idt, 0, sizeof(idt));
    idt_flush((uint32_t)&idt_ptr);
}
