#ifndef GDT_H
#define GDT_H
#include "../include/types.h"

typedef struct { uint16_t limit_lo, base_lo; uint8_t base_mid, access, gran, base_hi; }
    __attribute__((packed)) gdt_entry_t;
typedef struct { uint16_t limit; uint32_t base; }
    __attribute__((packed)) gdt_ptr_t;

void gdt_install(void);
#endif
