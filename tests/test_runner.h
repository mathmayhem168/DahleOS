#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H
/* Minimal assertion/reporting macros for host-side unit tests. */
#include <stdio.h>

static int _tr_pass = 0;
static int _tr_fail = 0;

#define ASSERT(desc, cond) \
    do { \
        if (cond) { \
            printf("  PASS  %s\n", (desc)); \
            _tr_pass++; \
        } else { \
            printf("  FAIL  %s  (line %d)\n", (desc), __LINE__); \
            _tr_fail++; \
        } \
    } while (0)

#define TEST_SUMMARY() \
    do { \
        printf("\n%d passed, %d failed.\n", _tr_pass, _tr_fail); \
        return (_tr_fail == 0) ? 0 : 1; \
    } while (0)

#endif
