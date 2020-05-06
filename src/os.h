#pragma once

#include <my-os/log2.h>
#include <my-os/types.h>
#include <stdarg.h>

/* #define MY_OS */

#ifdef MY_OS
#include <my-os/string.h>
#include "strtox.h"

static inline long long int my_strtoll(char *s, int base) {
    long long v;
    kstrtoll(s, base, &v);
    return v;
}

#define my_strtod(s) kstrtod(s)

#undef assert
#define assert(expr)

#else
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define my_strtod(s) strtod(s, NULL)
#define my_strtoll(s, base) strtoll(s, NULL, base)

#if !defined(MY_DEBUG)
#undef assert
#define assert(expr) assert(expr)
#endif // MY_DEBUG

#endif

void *my_malloc(size_t size);
void my_free(void *);
void *my_realloc(void *p, size_t size);
char *my_strdup(char *s);

int my_printf(const char *fmt, ...);
int my_sprintf(char *buf, const char *fmt, ...);
int my_vsprintf(char *buf, const char *fmt, va_list);
