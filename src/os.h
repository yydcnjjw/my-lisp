#pragma once

#include <stdarg.h>

#ifdef MY_OS
#include <my-os/string.h>
#include <my-os/types.h>
#else
#include <string.h>
#include <math.h>
#endif

void *my_malloc(size_t size);
void my_free(void *);
void *my_realloc(void *p, size_t size);
char *my_strdup(char *s);

void my_printf(const char *fmt, ...);
void my_sprintf(char *buf, const char *fmt, ...);
void my_vsprintf(char *buf, const char *fmt, va_list);
