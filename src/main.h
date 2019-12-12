#ifndef MAIN_H
#define MAIN_H

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>

#define IS_SPC 1
#define IS_ID 2
#define IS_NUM 4

#define TOK_HASH_SIZE 0x4000

typedef struct TokenSym {
    struct TokenSym *hash_next;
    int tok; /* token number */
    int len;
    char str[1];
} TokenSym;

#define CASE_SPACE                                                             \
    case ' ':                                                                  \
    case '\t':                                                                 \
    case '\v':                                                                 \
    case '\f':                                                                 \
    case '\r'

#define CASE_COMMONT case ';'

#define CASE_ID                                                                \
    case 'a' ... 'z':                                                          \
    case 'A' ... 'Z':                                                          \
    case '!':                                                                  \
    case '$':                                                                  \
    case '%':                                                                  \
    case '&':                                                                  \
    case '*':                                                                  \
    case '+':                                                                  \
    case '-':                                                                  \
    case '.':                                                                  \
    case '/':                                                                  \
    case ':':                                                                  \
    case '<':                                                                  \
    case '=':                                                                  \
    case '>':                                                                  \
    case '?':                                                                  \
    case '@':                                                                  \
    case '^':                                                                  \
    case '_':                                                                  \
    case '~'

#define CASE_NUMBER case '0' ... '9'

ssize_t file_read_all(char *filename, u_char **s_file);

#endif /* MAIN_H */
