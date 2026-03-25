#include "gdt.h"

#define GDT_ENTRIES 3
static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;
extern void gdt_flush(uint32_t);

static void set_gate(int n, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[n].base_lo  = base & 0xFFFF;
    gdt[n].base_mid = (base >> 16) & 0xFF;
    gdt[n].base_hi  = (base >> 24) & 0xFF;
    gdt[n].limit_lo = limit & 0xFFFF;
    gdt[n].gran     = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[n].access   = access;
}

void gdt_install(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)gdt;
    set_gate(0, 0, 0,          0x00, 0x00); /* null          */
    set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* ring-0 code   */
    set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* ring-0 data   */
    gdt_flush((uint32_t)&gdt_ptr);
}
