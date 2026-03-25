#ifndef STRING_H
#define STRING_H
#include "../include/types.h"

size_t strlen (const char *s);
char  *strcpy (char *d, const char *s);
char  *strncpy(char *d, const char *s, size_t n);
int    strcmp (const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcat (char *d, const char *s);
char  *strchr (const char *s, int c);

void int_to_str (int n, char *buf);
void uint_to_hex(uint32_t n, char *buf);
#endif
