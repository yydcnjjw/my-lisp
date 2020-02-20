%option noyyalloc noyyrealloc noyyfree
%option noyywrap nodefault 8bit
%option yylineno
%option reentrant bison-bridge bison-locations
/* %option never-interactive */
/* %option debug */
%option header-file="my_lisp.lex.h"

%{
#include "my_lisp.tab.h"
#include "my_lisp.h"
    void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s, ...);
    struct number_flag_t number_flag = {
        .flo = 0,
        .exact = 0,
        .complex = 0,
        .radix = 1 << RADIX_10,
        .naninf = 0
    };
    /* enum exact_flag exact_flag = -1; */
    enum radix_flag radix_flag = RADIX_10;
    enum exact_flag exact_flag = 0;
    u64 number_value[4] = {};
    u64 *number_tmp = number_value;
    char imaginary_sign = '\0';

    bool parse_real10_is_flo = false;
#define CUR_NUMBER_PARSE_PART_BIT (number_value == number_tmp ? _REAL_BIT : _IMAG_BIT)
%}

SPECIAL_INITIAL [!$%&*/:<=>?^_~]
LETTER [a-zA-Z]
INITIAL {SPECIAL_INITIAL}|{LETTER}
DIGIT [0-9]
SPECIAL_SUBSEQUENT [-+.@]
SUBSEQUENT ({INITIAL}|{DIGIT}|{SPECIAL_SUBSEQUENT})

PECULIAR_IDENTIFIER "+"|"-"|"..."|"->"{SUBSEQUENT}*
IDENTIFIER {INITIAL}{SUBSEQUENT}*

EOL	\n|\r\n|\n\r|\r
WS	{EOL}|[[:blank:]]

RADIX {RADIX2}|{RADIX8}|{RADIX10}|{RADIX16}
RADIX2 ("#"[bB])
RADIX8 ("#"[oO])
RADIX10 ("#"[dD])
RADIX16 ("#"[xX])

EXACTNESS_FIX ("#"[eE])
EXACTNESS_FLO ("#"[iI])
EXACTNESS {EXACTNESS_FIX}|{EXACTNESS_FLO}

EXPONENT_MARK [eEsSfFdDlL]

DIGIT10 [0-9]
DIGIT_ALL [0-9aAbBcCdDeEfF]

NAN "nan.0"
INF "inf.0"

UINTEGER {DIGIT_ALL}+
UREAL {UINTEGER}
_UREAL {UINTEGER}"/"{UINTEGER}
REAL ({SIGN}?{UREAL})
_REAL ({SIGN}?{_UREAL})

UINTEGER10 {DIGIT10}+
UREAL10 {UINTEGER10}
_UREAL10 {UINTEGER10}"/"{UINTEGER10}
_UREAL10_FLO (({DIGIT10}+"."{DIGIT10}*)|("."{DIGIT10}+))
REAL10 ({SIGN}?{UREAL10})
_REAL10 ({SIGN}?{_UREAL10})
_REAL10_FLO ({SIGN}?{_UREAL10_FLO})

SIGN [-+]

%x x_string x_character
%x x_prefix x_suffix x_mantissa_width x_real10_end
%x x_radix x_radix10
%x x_real x_real10 x_naninf
%x x_complex

%%
%{
  // TODO: support variable length
  #define MAX_STR_CONST 4096
  char string_buf[MAX_STR_CONST];
  char *string_buf_ptr;
%}

"(" { return LP; } // left parenthesis
")" { return RP; } // right parenthesis
"[" { return LSB; } // left square bracket
"]" { return RSB; } // right square bracket

"." { return PERIOD; } // period

"'" { return APOSTROPHE; } // apostrophe
"`" { return GRAVE; } // grave
"," { return COMMA; } // comma
",@" { return COMMA_AT; } // comma at

"#'" { return NS_APOSTROPHE; } // number sign apostrophe
"#`" { return NS_GRAVE; } // number sign grave
"#," { return NS_COMMA; } // number sign comma
"#,@" { return NS_COMMA_AT; } // number sign comma at
"#("    { return VECTOR_LP; }
"#vu8(" { return VECTOR_BYTE_LP; }

{IDENTIFIER} |
"+" |
"-" |
"..." |
"->"{SUBSEQUENT}* { yylval->symbol = lookup(yyextra, yytext); return IDENTIFIER; }

\" { string_buf_ptr = string_buf; BEGIN(x_string); }
<x_string>{
\\a { *string_buf_ptr++ = '\n'; }
\\b { *string_buf_ptr++ = '\b'; }
\\t { *string_buf_ptr++ = '\t'; }
\\n { *string_buf_ptr++ = '\n'; }
\\v { *string_buf_ptr++ = '\v'; }
\\f { *string_buf_ptr++ = '\f'; }
\\r { *string_buf_ptr++ = '\r'; }
\\\" { *string_buf_ptr++ = '\"'; }
\\\\ { *string_buf_ptr++ = '\\'; }

<<EOF>> { yyerror(yylloc, yyscanner, yyextra, "the string misses \" to terminate before EOF"); }
[^\\\"]+ { char *yptr = yytext; while (*yptr) *string_buf_ptr++ = *yptr++;  }
\" {
    BEGIN(INITIAL);
    yylval->str = make_string(string_buf, string_buf_ptr - string_buf);
    return STRING;
}
}

<INITIAL,x_prefix>{
({RADIX}{EXACTNESS}?)|({EXACTNESS}{RADIX}?) {
    if (yyleng > 0) {
        set_number_prefix(&number_flag, yytext[1], &exact_flag);
    }
    if (yyleng > 2) {
        set_number_prefix(&number_flag, yytext[3], &exact_flag);
    }

    int radix = number_flag.radix;
    radix_flag = ilog2(radix);
    if (radix_flag == RADIX_10) {
        BEGIN(x_real10);
    } else {
        BEGIN(x_real);
    }
  }
}

<INITIAL,x_naninf,x_real10,x_real>{
"+"{NAN} {
    if (number_value == number_tmp) {
        number_flag.naninf = NAN_POSITIVE;
    } else {
        number_flag.naninf |= NAN_POSITIVE << 4;
    }
    BEGIN(x_complex);
}

"-"{NAN} {
    if (number_value == number_tmp) {
        number_flag.naninf = NAN_NEGATIVE;
    } else {
        number_flag.naninf |= NAN_NEGATIVE << 4;
    }
    BEGIN(x_complex);
}
"+"{INF} {
    if (number_value == number_tmp) {
        number_flag.naninf = INF_POSITIVE;
    } else {
        number_flag.naninf |= INF_POSITIVE << 4;
    }
    BEGIN(x_complex);
}
"-"{INF} {
    if (number_value == number_tmp) {
        number_flag.naninf = INF_NEGATIVE;
    } else {
        number_flag.naninf |= INF_NEGATIVE << 4;
    }
    BEGIN(x_complex);
}
}

<INITIAL,x_real10>{
{REAL10} {
    number_tmp[0] = _str_to_real(yytext, RADIX_10, false);
    BEGIN(x_suffix);
}

{_REAL10_FLO} {
    number_tmp[0] = _str_to_real(yytext, RADIX_10, true);
    parse_real10_is_flo = true;
    BEGIN(x_suffix);
}

{_REAL10} {
    extract_uinteger(yytext, number_tmp, RADIX_10);
    if (number_flag.flo) {
        _exact_to_flo(number_tmp, true);
    } else {
        number_flag.exact = number_tmp == number_value ? _REAL_BIT : _IMAG_BIT;
    }

    BEGIN(x_complex);
}
}

<x_suffix>{
{EXPONENT_MARK}{SIGN}?{DIGIT10}+ {
    u64 num = _str_to_real(yytext+1, RADIX_10, false);
    s64 exp = TO_TYPE(num, s64);
    exp = (s64)pow(10, exp);

    if (!parse_real10_is_flo) {
        s64 n = TO_TYPE(number_tmp[0], s64) * exp;
        TYPE_CPY(number_tmp[0], n);
    } else {
        double n = TO_TYPE(number_tmp[0], double) * exp;
        TYPE_CPY(number_tmp[0], n);
    }
    BEGIN(x_mantissa_width);
}

.|\n {
    yyless(yyleng - 1); BEGIN(x_mantissa_width);
}
}

<x_mantissa_width>{
"|"{DIGIT10}+ {
    BEGIN(x_real10_end);
}
.|\n {
    yyless(yyleng-1); BEGIN(x_real10_end);
}
}

<x_real10_end>{
.|\n {
    u8 part_bit = number_value == number_tmp ? _REAL_BIT : _IMAG_BIT;
    if (parse_real10_is_flo) {
        if (exact_flag == EXACT) {
            _flo_to_exact(number_tmp);
        } else {
            if (part_bit & _IMAG_BIT && !number_flag.flo) {
                if (number_flag.exact & _REAL_BIT) {
                    number_flag.exact = 0;
                    _exact_to_flo(number_value, true);
                } else {
                    _exact_to_flo(number_value, false);
                }
            }
            number_flag.flo = true;
        }
        parse_real10_is_flo = false;
    } else {
        if (number_flag.flo) {
            _exact_to_flo(number_tmp, false);
        }
    }
    yyless(yyleng-1);
    BEGIN(x_complex);
}
}

<x_real>{
{REAL} {
    assert(radix_flag != RADIX_10);
    number_tmp[0] = _str_to_real(yytext, radix_flag, false);
    if (number_flag.flo) {
        _exact_to_flo(number_tmp, false);
    }
    BEGIN(x_complex);
}

{_REAL} {
    assert(radix_flag != RADIX_10);
    extract_uinteger(yytext, number_tmp, radix_flag);
    if (number_flag.flo) {
        _exact_to_flo(number_tmp, true);
    } else {
        number_flag.exact = number_tmp == number_value ? _REAL_BIT : _IMAG_BIT;
    }
    BEGIN(x_complex);
}
}

<INITIAL,x_real,x_real10>{
{SIGN}"i" {
    s64 n = 1;
    if (yytext[0] == '-') {
        n = -1;
    }
    if (number_tmp == number_value) {
        number_tmp = number_value + 2;
    }
    TYPE_CPY(number_tmp[0], n);
    if (number_flag.flo) {
        _exact_to_flo(number_tmp, false);
    }
    yyless(yyleng - 1);
    BEGIN(x_complex);
}
}

<x_complex>{
"@" {
    if (imaginary_sign != '\0') {
        yyless(yyleng - 1);
        BEGIN(INITIAL);
    }

    if (radix_flag == RADIX_10) {
        BEGIN(x_real10);
    } else {
        BEGIN(x_real);
    }
}
"+"|"-" {
    if (imaginary_sign != '\0') {
        yyless(yyleng - 1);
        BEGIN(INITIAL);
    }
    imaginary_sign = yytext[0];

    number_tmp = number_value + 2;
    yyless(yyleng-1);
    if (radix_flag == RADIX_10) {
        BEGIN(x_real10);
    } else {
        BEGIN(x_real);
    }

}

"i" {
    number_flag.complex = 1;
    if (imaginary_sign == '\0') {
        // [+-]\d*i
        memcpy(number_value + 2, number_value, sizeof(u64) * 2);
        memset(number_value, 0, sizeof(u64) * 2);
    }
}

.|\n {
    if (imaginary_sign != '\0' && !number_flag.complex) {
        yyerror(yylloc, yyscanner, yyextra, "pasre error");
        assert(!number_flag.complex);
    }
    yyless(yyleng - 1);
    BEGIN(INITIAL);

    yylval->num = make_number(number_flag, number_value);

    bzero(&number_flag, sizeof(struct number_flag_t));
    number_flag.radix = 1 << RADIX_10;
    number_flag.exact = 0;
    radix_flag = RADIX_10;
    exact_flag = 0;
    bzero(number_value, sizeof(u64) * 4);
    number_tmp = number_value;
    imaginary_sign = '\0';
    return NUMBER;
}
}


"#\\" { BEGIN(x_character); }
<x_character>{
"nul" {
    BEGIN(INITIAL);
    yylval->ch = '\v';
    return CHARACTER;
}
"alarm" {
    BEGIN(INITIAL);
    yylval->ch = '\v';
    return CHARACTER;
}
"backspace" { BEGIN(INITIAL); yylval->ch = 'a';  return CHARACTER; }
"tab" { BEGIN(INITIAL); yylval->ch = 'a'; return CHARACTER; }
"linefeed" { BEGIN(INITIAL); yylval->ch = 'a';  return CHARACTER; }
"newline" { BEGIN(INITIAL); yylval->ch = 'a'; return CHARACTER; }
"vtab" { BEGIN(INITIAL); yylval->ch = 'a'; return CHARACTER; }
"page" { BEGIN(INITIAL); yylval->ch = 'a'; return CHARACTER; }
"return" { BEGIN(INITIAL); yylval->ch = 'a'; return CHARACTER; }
"esc" { BEGIN(INITIAL); yylval->ch = 'a'; return CHARACTER; }
"space" { BEGIN(INITIAL); yylval->ch = 'a'; return CHARACTER; }
"delete" { BEGIN(INITIAL); yylval->ch = 'a'; return CHARACTER; }
}

"#T" |
"#t" { return BOOLEAN_T; }

"#F" |
"#f" { return BOOLEAN_F; }

;.*$
{WS}* { }

<<EOF>> { return END_OF_FILE; }

. { yyerror(yylloc, yyscanner, yyextra, "Mystery character %c\n", *yytext); }
%%
