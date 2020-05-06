#include <os.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int my_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}

int my_vsprintf(char *buf, const char *fmt, va_list va) {
    return vsprintf(buf, fmt, va);
}

int my_sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = my_vsprintf(buf, fmt, args);
    va_end(args);
    return ret;
}

void *my_malloc(size_t size) {
    void *ret = malloc(size);
    if (!ret) {
        my_printf("malloc error\n");
        /* exit(0); */
    }
    bzero(ret, size);
    return ret;
}

void my_free(void *o) {
    if (o) {
        free(o);
    }
}

void *my_realloc(void *p, size_t size) {
    if (!p) {
        return my_malloc(size);
    }
    return realloc(p, size);
}

void *yyalloc(size_t bytes, void *yyscanner) { return my_malloc(bytes); }

void *yyrealloc(void *ptr, size_t bytes, void *yyscanner) {
    return my_realloc(ptr, bytes);
}

void yyfree(void *ptr, void *yyscanner) { my_free(ptr); }

char *my_strdup(char *s) { return strdup(s); }
