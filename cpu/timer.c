#include "timer.h"
#include "irq.h"
#include "../drivers/port.h"

static volatile uint32_t ticks = 0;

static void on_tick(registers_t *r) { (void)r; ticks++; }

void timer_install(uint32_t hz) {
    if (!hz) hz = 1;
    register_interrupt_handler(IRQ0, on_tick);
    uint32_t div = 1193180 / hz;
    port_byte_out(0x43, 0x36);
    port_byte_out(0x40, (uint8_t)(div & 0xFF));
    port_byte_out(0x40, (uint8_t)(div >> 8));
}

uint32_t timer_ticks(void) { return ticks; }
