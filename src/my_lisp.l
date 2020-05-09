%option noyyalloc noyyrealloc noyyfree
%option noyywrap nodefault 8bit
%option yylineno
%option reentrant bison-bridge bison-locations
/* %option never-interactive */
%option debug
%option header-file="my_lisp.lex.h"

%{
#include "my_lisp.h"
    void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s, ...);
    number_full_t number_full = {
        .prefix = {
            .exact_type = EXACT_UNDEFINE,
            .radix_type = RADIX_10
        },
        .complex = {
            .real = {
                .type = NUMBER_PART_NONE
            },
            .imag = {
                .type = NUMBER_PART_NONE
            }
        }
    };
    // XXX: can not use this function
    /* lex_number_full_init(&number_full); */
    enum complex_part_t cur_parse_part = COMPLEX_PART_REAL;
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
%x x_prefix x_suffix x_mantissa_width
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
    lex_number_populate_prefix_from_str(yytext, yyleng, &number_full.prefix);

    if (number_full.prefix.radix_type == RADIX_10) {
        BEGIN(x_real10);
    } else {
        BEGIN(x_real);
    }
  }
}

<INITIAL,x_naninf,x_real10,x_real>{
"+"{NAN} {
    lex_number_populate_part_naninf(&number_full, cur_parse_part, NAN_POSITIVE);
    BEGIN(x_complex);
}

"-"{NAN} {
    lex_number_populate_part_naninf(&number_full, cur_parse_part, NAN_NEGATIVE);
    BEGIN(x_complex);
}
"+"{INF} {
    lex_number_populate_part_naninf(&number_full, cur_parse_part, INF_POSITIVE);
    BEGIN(x_complex);
}
"-"{INF} {
    lex_number_populate_part_naninf(&number_full, cur_parse_part, INF_NEGATIVE);
    BEGIN(x_complex);
}
}

<INITIAL,x_real10>{
{REAL10} {
    lex_number_populate_part_from_str(&number_full, cur_parse_part, yytext, NUMBER_PART_ZIP_EXACT);
    BEGIN(x_suffix);
}

{_REAL10_FLO} {
    lex_number_populate_part_from_str(&number_full, cur_parse_part, yytext, NUMBER_PART_FLO);
    BEGIN(x_suffix);
}

{_REAL10} {
    lex_number_populate_part_from_str(&number_full, cur_parse_part, yytext, NUMBER_PART_EXACT);
    BEGIN(x_complex);
}
}

<x_suffix>{
{EXPONENT_MARK}{SIGN}?{DIGIT10}+ {
    int ret = lex_number_calc_part_exp_from_str(&number_full, cur_parse_part, yytext + 1);
    if (ret < 0) {
        yyerror(yylloc, yyscanner, yyextra, "parse exp failure");
    }
    BEGIN(x_mantissa_width);
}

.|\n {
    yyless(yyleng - 1); BEGIN(x_mantissa_width);
}
}

<x_mantissa_width>{
"|"{DIGIT10}+ {
    BEGIN(x_complex);
}
.|\n {
    yyless(yyleng-1);
    BEGIN(x_complex);
}
}

<x_real>{
{REAL} {
    lex_number_populate_part_from_str(&number_full, cur_parse_part, yytext, NUMBER_PART_ZIP_EXACT);    
    BEGIN(x_complex);
}

{_REAL} {
    lex_number_populate_part_from_str(&number_full, cur_parse_part, yytext, NUMBER_PART_EXACT);
    BEGIN(x_complex);
}
}

<INITIAL,x_real,x_real10>{
{SIGN}"i" {
    cur_parse_part = COMPLEX_PART_IMAG;
    lex_number_populate_imag_part_one_i(&number_full, yytext[0]);
    yyless(yyleng - 1);
    BEGIN(x_complex);
}
}

<x_complex>{
"@" {
    if (number_full.prefix.radix_type == RADIX_10) {
        BEGIN(x_real10);
    } else {
        BEGIN(x_real);
    }
}
"+"|"-" {
    cur_parse_part = COMPLEX_PART_IMAG;
    yyless(yyleng-1);
    if (number_full.prefix.radix_type == RADIX_10) {
        BEGIN(x_real10);
    } else {
        BEGIN(x_real);
    }
}

"i" {
}

.|\n {
    yyless(yyleng - 1);
    BEGIN(INITIAL);

    yylval->num = make_number_from_full(&number_full);
    lex_number_full_init(&number_full);
    cur_parse_part = COMPLEX_PART_REAL;
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
