#ifndef MEM_H
#define MEM_H
#include "../include/types.h"
void *memset(void *p, int v, size_t n);
void *memcpy(void *d, const void *s, size_t n);
int   memcmp(const void *a, const void *b, size_t n);
#endif
