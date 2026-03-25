#include "port.h"

uint8_t port_byte_in(uint16_t p) {
    uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r;
}
void port_byte_out(uint16_t p, uint8_t d) {
    __asm__ volatile("outb %0,%1"::"a"(d),"Nd"(p));
}
uint16_t port_word_in(uint16_t p) {
    uint16_t r; __asm__ volatile("inw %1,%0":"=a"(r):"Nd"(p)); return r;
}
void port_word_out(uint16_t p, uint16_t d) {
    __asm__ volatile("outw %0,%1"::"a"(d),"Nd"(p));
}
uint32_t port_long_in(uint16_t p) {
    uint32_t r; __asm__ volatile("inl %1,%0":"=a"(r):"Nd"(p)); return r;
}
void port_long_out(uint16_t p, uint32_t d) {
    __asm__ volatile("outl %0,%1"::"a"(d),"Nd"(p));
}
