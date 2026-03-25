#ifndef TIMER_H
#define TIMER_H
#include "../include/types.h"
void     timer_install(uint32_t hz);
uint32_t timer_ticks(void);
#endif
