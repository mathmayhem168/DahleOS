#include "mem.h"

void *memset(void *p, int v, size_t n) {
    unsigned char *b = p;
    while (n--) *b++ = (unsigned char)v;
    return p;
}
void *memcpy(void *d, const void *s, size_t n) {
    unsigned char *dd = d;
    const unsigned char *ss = s;
    while (n--) *dd++ = *ss++;
    return d;
}
int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = a, *y = b;
    while (n--) { if (*x != *y) return *x - *y; x++; y++; }
    return 0;
}
