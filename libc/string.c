#include "string.h"

size_t strlen(const char *s) { size_t n=0; while(s[n]) n++; return n; }

char *strcpy(char *d, const char *s) {
    char *r=d; while((*d++=*s++)); return r;
}
char *strncpy(char *d, const char *s, size_t n) {
    size_t i; for(i=0;i<n&&s[i];i++) d[i]=s[i]; for(;i<n;i++) d[i]=0; return d;
}
int strcmp(const char *a, const char *b) {
    while(*a&&(*a==*b)){a++;b++;} return (unsigned char)*a-(unsigned char)*b;
}
int strncmp(const char *a, const char *b, size_t n) {
    while(n&&*a&&(*a==*b)){a++;b++;n--;} if(!n)return 0;
    return (unsigned char)*a-(unsigned char)*b;
}
char *strcat(char *d, const char *s) {
    char *r=d; while(*d) d++; while((*d++=*s++)); return r;
}
char *strchr(const char *s, int c) {
    while(*s){if(*s==(char)c)return(char*)s;s++;} return NULL;
}

void int_to_str(int n, char *buf) {
    if(!n){buf[0]='0';buf[1]=0;return;}
    char t[12]; int i=0,neg=0;
    if(n<0){neg=1;n=-n;}
    while(n>0){t[i++]='0'+(n%10);n/=10;}
    if(neg)t[i++]='-';
    int j=0; while(i>0) buf[j++]=t[--i]; buf[j]=0;
}

void uint_to_hex(uint32_t n, char *buf) {
    const char *h="0123456789ABCDEF";
    buf[0]='0'; buf[1]='x';
    for(int i=0;i<8;i++) buf[2+i]=h[(n>>(28-i*4))&0xF];
    buf[10]=0;
}
