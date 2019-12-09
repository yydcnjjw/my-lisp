#include <stddef.h>
#include <stdio.h>

char ch;
int line_num;
FILE *fp;
char *tokcstr;

void get_token();
void get_ch();

void get_ch() { ch = getc(fp); }

int is_nodigit(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int is_digit(char c) { return c >= '0' && c <= '9'; }

enum parse_status { PARSE_NONE, PARSE_IDENTIFIER } parse_status;

void parse_identifier() {
    if (parse_status != PARSE_IDENTIFIER) {
    }
}

void parse_comment() {
    while (ch != '\n') {
        get_ch();
        printf("%c", ch);
    }
}

void skip_white_space() {
    while (ch == ' ' || ch == '\t' || ch == '\r') {
        if (ch == '\r') {
            get_ch();
            if (ch != '\n')
                return;
            line_num++;
        }
        printf("%c", ch);
        get_ch();
    }
}

void preprocess() {
    for (;;) {
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            skip_white_space();
        } else if (ch == ';') {
            parse_comment();
        } else {
            break;
        }
    }
}

void get_token() {
    preprocess();
    parse_status = PARSE_NONE;
    switch (ch) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '0' ... '9':
    case '!':
    case '$':
    case '%':
    case '&':
    case '*':
    case '+':
    case '-':
    case '.':
    case '/':
    case ':':
    case '<':
    case '=':
    case '>':
    case '?':
    case '@':
    case '^':
    case '_':
    case '~': {
        parse_status = PARSE_IDENTIFIER;
        parse_identifier();
        break;
    }
    case '(':
    case ')':
        break;

    default:
        break;
    }
}

int main(int argc, char *argv[]) { return 0; }
