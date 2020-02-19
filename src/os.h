#pragma once

#include <stdarg.h>
#include <my-os/types.h>
#include <my-os/log2.h>

#ifdef MY_OS
#include <my-os/string.h>
#else
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#if !defined (MY_DEBUG)
#undef assert
#define assert(expr)
#endif // MY_DEBUG

#endif

void *my_malloc(size_t size);
void my_free(void *);
void *my_realloc(void *p, size_t size);
char *my_strdup(char *s);

int my_printf(const char *fmt, ...);
int my_sprintf(char *buf, const char *fmt, ...);
int my_vsprintf(char *buf, const char *fmt, va_list);
