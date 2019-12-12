#include "main.h"

#include <sys/types.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct File {
    u_char *buf_ptr;
    size_t file_len;
} File;

typedef union CValue {
    double d;
    float f;
    uint64_t i;
    struct {
        int size;
        const void *data;
    } str;
} CValue;

#define TOK_IDENT 256
#define TOK_ALLOC_INCR 512

File *file;
int line_num;
int tok;
CValue tokc;
static uint8_t isidnum_table[256 - EOF];
static TokenSym **table_ident;
int tok_ident;
static TokenSym *hash_ident[TOK_HASH_SIZE];

void get_token();

void parse_identifier() {}

bool is_idnum_table(u_char ch, uint8_t type) {
    return isidnum_table[ch - EOF] & type;
}

u_char *skip_white_space(u_char *buf_ptr) {
    while (isidnum_table[*buf_ptr - EOF] & IS_SPC)
        buf_ptr++;
    return buf_ptr;
}

u_char *parse_commont(u_char *buf_ptr) {
    while (*buf_ptr != '\n')
        buf_ptr++;
    return buf_ptr;
}

#define TOK_HASH_INIT 1
#define TOK_HASH_FUNC(h, c) ((h) + ((h) << 5) + ((h) >> 27) + (c))

static TokenSym *tok_alloc_new(const u_char *str, size_t len) {
    int i = tok_ident - TOK_IDENT;
    if ((i % TOK_ALLOC_INCR) == 0) {
        table_ident =
            realloc(table_ident, (i + TOK_ALLOC_INCR) * sizeof(TokenSym *));
    }

    TokenSym *ts = malloc(sizeof(TokenSym) + len);
    table_ident[i] = ts;
    ts->tok = tok_ident++;
    bzero(ts, sizeof(TokenSym) + len);
    ts->len = len;
    memcpy(ts->str, str, len);
    ts->str[len] = '\0';
    return ts;
}

/* TokenSym *tok_alloc(const u_char *str, int len) { */

/* } */

u_char *parse_ident(u_char *buf_ptr) {
    u_char *p = buf_ptr;
    uint h = TOK_HASH_INIT;
    h = TOK_HASH_FUNC(h, *p);
    while (++p, is_idnum_table(*p, IS_ID | IS_NUM))
        h = TOK_HASH_FUNC(h, *p);
    size_t len = p - buf_ptr;

    h &= (TOK_HASH_SIZE - 1);
    TokenSym **pts;
    TokenSym *ts;
    *pts = hash_ident[h];

    for (;;) {
        ts = *pts;
        if (!ts) {
            // token alloc new
            ts = tok_alloc_new(buf_ptr, len);
            *pts = ts;
            break;
        }
        if (ts->len == len && !memcmp(ts->str, buf_ptr, len)) {
            // found
            break;
        }
        pts = &(ts->hash_next);
    }
    tok = ts->tok;
    return p;
}

void get_token() {
    u_char c;
    u_char *p, *p1;
    p = file->buf_ptr;
redo_on_start:
    c = *p;
    switch (c) {
    CASE_SPACE : {
        p = skip_white_space(p);
        goto redo_on_start;
    }
    CASE_COMMONT : {

        p = parse_commont(p);
        goto redo_on_start;
    }
    CASE_ID : {
        p = parse_ident(p);
        break;
    }
    case '(':
    case ')':
    case '[':
    case ']':
    case '`':
    case ',':
    case '\'':
        tok = c;
        break;

    default:
        printf("unrecognized character\n");
        break;
    }
    file->buf_ptr = p;
}

#define SWITCH_IF(v, CASE, boolean)                                            \
    do {                                                                       \
        switch (v) {                                                           \
        CASE : {                                                               \
            boolean = true;                                                    \
            break;                                                             \
        }                                                                      \
        default:                                                               \
            boolean = false;                                                   \
            break;                                                             \
        }                                                                      \
    } while (0)

static inline int is_space(char ch) {
    bool val;
    SWITCH_IF(ch, CASE_SPACE, val);
    return val;
}

static inline bool isid(char ch) {
    bool val;
    SWITCH_IF(ch, CASE_ID, val);
    return val;
}

static inline bool isnum(char c) { return c >= '0' && c <= '9'; }
static inline bool isoct(char c) { return c >= '0' && c <= '7'; }

static inline u_char toup(char c) {
    return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c;
}

u_char set_idnum(char c, u_char val) {
    u_char prev = isidnum_table[c - EOF];
    isidnum_table[c - EOF] = val;
    return prev;
}

void init() {
    int i;
    for (i = EOF; i < 128; i++) {
        set_idnum(i, is_space(i) ? IS_SPC
                                 : isid(i) ? IS_ID : isnum(i) ? IS_NUM : 0);
    }

    for (i = 128; i < 256; i++) {
        set_idnum(i, IS_ID);
    }
}

int main(int argc, char *argv[]) {
    file = malloc(sizeof(File));
    file->file_len = file_read_all("test.ss", &file->buf_ptr);
    init();
    return 0;
}
