/* =============================================================
   tests/test_graphics.c  —  Host-side unit tests for the
   'graphics' command argument parser.

   Compiled with the system C compiler (not the cross-compiler)
   and run natively on the build machine via `make test`.
   No kernel or QEMU required.

   The file only tests shell/graphics_parse.h, which is a
   self-contained inline parser with no kernel dependencies.
   ============================================================= */

#include "../shell/graphics_parse.h"
#include "test_runner.h"

/* ── helpers ─────────────────────────────────────────────── */

/* Shorthand: parse with a throwaway mode_out pointer. */
static int parse(const char *args) {
    const char *dummy;
    return graphics_parse(args, &dummy);
}

/* Parse and capture the mode token. */
static int parse_m(const char *args, const char **m) {
    return graphics_parse(args, m);
}

/* ── test groups ─────────────────────────────────────────── */

static void test_null_and_empty(void) {
    ASSERT("NULL args → USAGE",         parse(NULL) == GFXP_USAGE);
    ASSERT("empty string → USAGE",      parse("")   == GFXP_USAGE);
    ASSERT("whitespace only → USAGE",   parse(" ")  == GFXP_USAGE);
}

static void test_status(void) {
    ASSERT("'status' → STATUS",         parse("status") == GFXP_STATUS);
    ASSERT("'statusX' → USAGE",         parse("statusX") == GFXP_USAGE);
    ASSERT("'Status' (wrong case) → USAGE", parse("Status") == GFXP_USAGE);
}

static void test_set_vbe(void) {
    const char *m;
    ASSERT("'set vbe' → SET_VBE",       parse_m("set vbe", &m) == GFXP_SET_VBE);
    ASSERT("'set  vbe' (extra space)",  parse("set  vbe")  == GFXP_SET_VBE);
    ASSERT("'set   vbe' (triple space)",parse("set   vbe") == GFXP_SET_VBE);
}

static void test_set_vga(void) {
    ASSERT("'set vga' → SET_VGA",       parse("set vga")  == GFXP_SET_VGA);
    ASSERT("'set  vga' (extra space)",  parse("set  vga") == GFXP_SET_VGA);
}

static void test_set_unknown(void) {
    const char *m;
    ASSERT("'set xyz' → SET_UNKNOWN",   parse_m("set xyz", &m) == GFXP_SET_UNKNOWN);
    ASSERT("'set xyz' mode_out='xyz'",  gfx_streq(m, "xyz"));

    ASSERT("'set ' (empty mode) → SET_UNKNOWN", parse_m("set ", &m) == GFXP_SET_UNKNOWN);
    ASSERT("'set ' mode_out is empty string",   gfx_streq(m, ""));

    ASSERT("'set VBE' (wrong case) → SET_UNKNOWN", parse("set VBE") == GFXP_SET_UNKNOWN);
    ASSERT("'set VGA' (wrong case) → SET_UNKNOWN", parse("set VGA") == GFXP_SET_UNKNOWN);
}

static void test_no_space_after_set(void) {
    /* "setvbe" has no space — must not be treated as 'set vbe' */
    ASSERT("'setvbe' → USAGE",          parse("setvbe")  == GFXP_USAGE);
    ASSERT("'setvga' → USAGE",          parse("setvga")  == GFXP_USAGE);
    ASSERT("'set' alone → USAGE",       parse("set")     == GFXP_USAGE);
}

static void test_test_subcommand(void) {
    ASSERT("'test' → TEST",             parse("test")  == GFXP_TEST);
    ASSERT("'testX' → USAGE",           parse("testX") == GFXP_USAGE);
}

static void test_unknown_subcommands(void) {
    const char *m;
    ASSERT("'foo' → USAGE",                     parse("foo")       == GFXP_USAGE);
    ASSERT("'help' → USAGE",                    parse("help")      == GFXP_USAGE);
    /* "set vbe extra" — the token after "set " is "vbe extra", which is not
       a valid mode name, so it returns SET_UNKNOWN (not USAGE) */
    ASSERT("'set vbe extra' → SET_UNKNOWN",     parse_m("set vbe extra", &m) == GFXP_SET_UNKNOWN);
    ASSERT("'set vbe extra' mode_out='vbe extra'", gfx_streq(m, "vbe extra"));
}

/* ── main ────────────────────────────────────────────────── */

int main(void) {
    printf("graphics command — unit tests\n");
    printf("------------------------------\n");

    test_null_and_empty();
    test_status();
    test_set_vbe();
    test_set_vga();
    test_set_unknown();
    test_no_space_after_set();
    test_test_subcommand();
    test_unknown_subcommands();

    TEST_SUMMARY();
}
