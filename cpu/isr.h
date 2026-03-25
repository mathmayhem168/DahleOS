#ifndef ISR_H
#define ISR_H
#include "../include/types.h"

/*
 * Register snapshot as it sits on the stack when an ISR fires.
 *
 * Push sequence (low → high address):
 *   push ds         ← offset  0
 *   pusha           ← edi(4) esi(8) ebp(12) esp(16) ebx(20) edx(24) ecx(28) eax(32)
 *   push int_no     ← offset 36
 *   push err_code   ← offset 40
 *   CPU: eip(44) cs(48) eflags(52) [useresp(56) ss(60) if ring change]
 */
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*isr_t)(registers_t *);

extern isr_t interrupt_handlers[256];   /* shared with IRQ layer */

void isr_install(void);
void register_interrupt_handler(uint8_t n, isr_t fn);

#endif
