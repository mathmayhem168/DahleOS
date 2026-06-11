#ifndef SHELL_GRAPHICS_PARSE_H
#define SHELL_GRAPHICS_PARSE_H

/* Return codes from graphics_parse() */
#define GFXP_USAGE        0   /* no/unknown args → print usage */
#define GFXP_STATUS       1   /* "status"                       */
#define GFXP_SET_VBE      2   /* "set vbe"                      */
#define GFXP_SET_VGA      3   /* "set vga"                      */
#define GFXP_SET_UNKNOWN  4   /* "set <other>"                  */
#define GFXP_TEST         5   /* "test"                         */

/* String equality — avoids pulling in libc/string.h or <string.h>
   so this header is usable in both the kernel and host-side tests. */
static inline int gfx_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == '\0' && *b == '\0');
}

/* Classify the args string for the 'graphics' command.
   On GFXP_SET_UNKNOWN *mode_out is set to the unrecognised token;
   on all other codes *mode_out is set to NULL.
   Pure function: no I/O, no side-effects. */
static inline int graphics_parse(const char *args, const char **mode_out) {
    *mode_out = (const char *)0;

    if (!args || !*args) return GFXP_USAGE;

    if (gfx_streq(args, "status")) return GFXP_STATUS;
    if (gfx_streq(args, "test"))   return GFXP_TEST;

    /* "set <mode>" — require a space after "set" */
    if (args[0]=='s' && args[1]=='e' && args[2]=='t' && args[3]==' ') {
        const char *t = args + 4;
        while (*t == ' ') t++;          /* allow extra spaces */
        *mode_out = t;
        if (gfx_streq(t, "vbe")) return GFXP_SET_VBE;
        if (gfx_streq(t, "vga")) return GFXP_SET_VGA;
        return GFXP_SET_UNKNOWN;
    }

    return GFXP_USAGE;
}

#endif
