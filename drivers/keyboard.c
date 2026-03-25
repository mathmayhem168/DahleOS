#include "keyboard.h"
#include "port.h"
#include "../cpu/irq.h"
#include "../cpu/isr.h"

#define BUF 256

static char     buf[BUF];
static volatile int head = 0, tail = 0;
static int shift = 0, caps = 0;

/* US QWERTY scancode set 1 */
static const char norm[] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};
static const char shft[] = {
    0,27,'!','@','#','$','%','^','&','*','(',')', '_', '+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
};

static void kbd_handler(registers_t *r) {
    (void)r;
    uint8_t sc = port_byte_in(0x60);

    if (sc == 0x2A || sc == 0x36) { shift = 1; return; }
    if (sc == 0xAA || sc == 0xB6) { shift = 0; return; }
    if (sc == 0x3A) { caps = !caps; return; }
    if (sc & 0x80) return; /* key release, ignore */

    char c = 0;
    if (sc < (uint8_t)sizeof(norm))
        c = (shift ^ caps) ? shft[sc] : norm[sc];
    if (!c) return;

    int next = (head + 1) % BUF;
    if (next != tail) { buf[head] = c; head = next; }
}

void keyboard_install(void) {
    register_interrupt_handler(IRQ1, kbd_handler);
}

char keyboard_getchar(void) {
    while (head == tail) __asm__ volatile("hlt");
    char c = buf[tail];
    tail = (tail + 1) % BUF;
    return c;
}

int keyboard_poll(void) { return head != tail; }
