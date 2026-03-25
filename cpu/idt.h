#ifndef IDT_H
#define IDT_H
#include "../include/types.h"

typedef struct { uint16_t base_lo, sel; uint8_t zero, flags; uint16_t base_hi; }
    __attribute__((packed)) idt_entry_t;
typedef struct { uint16_t limit; uint32_t base; }
    __attribute__((packed)) idt_ptr_t;

void idt_set_gate(uint8_t n, uint32_t base, uint16_t sel, uint8_t flags);
void idt_install(void);
#endif
