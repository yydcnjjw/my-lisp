%option noyywrap nodefault 8bit
%option yylineno
%option reentrant bison-bridge bison-locations
/* %option debug */
%option header-file="my_lisp.lex.h"

%{
#include "my_lisp.tab.h"
#include "my_lisp.h"
#include <math.h>

    enum radix radix_flag = RADIX_10;
    enum exactness exactness_flag = EXACTNESS_UNKOWN;
    enum exactness exactness_derivation = EXACTNESS_FIX;
    u64 real_part = 0;
    u64 imaginary_part = 0;
    u64 tmp_number = 0;
    char imaginary_sign = '\0';

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
NANINF ({NAN}|{INF})
REAL_NANINF {SIGN}{NANINF}

UINTEGER {DIGIT_ALL}+
UREAL {UINTEGER}
_UREAL {UINTEGER}"/"{UINTEGER}
REAL ({SIGN}{UREAL})
_REAL ({SIGN}{_UREAL})

UINTEGER10 {DIGIT10}+
UREAL10 {UINTEGER10}
_UREAL10 {UINTEGER10}"/"{UINTEGER10}
_UREAL10_FLO (({DIGIT10}+"."{DIGIT10}*)|("."{DIGIT10}+))
REAL10 ({SIGN}{UREAL10})
_REAL10 ({SIGN}{_UREAL10})
_REAL10_FLO ({SIGN}{_UREAL10_FLO})

SIGN [-+]

%x string
%x prefix suffix mantissa_width
%x radix radix10
%x real real10 ureal ureal10 unaninf naninf
%x complex

%%    
%{
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

\" { string_buf_ptr = string_buf; BEGIN(string); }
<string>{
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

<INITIAL,prefix>{
{RADIX}{EXACTNESS}? {
    if (yyleng > 2) {
        exactness_flag = to_exactness_flag(yytext[3]);
    }

    radix_flag = to_radix_flag(yytext[1]);

    if (radix_flag == RADIX_10) {
        BEGIN(real10);
    } else {
        BEGIN(real);
    }
  }

{EXACTNESS}{RADIX}? {
    if (yyleng > 2) {
        radix_flag = to_radix_flag(yytext[3]);
    }
    exactness_flag = to_exactness_flag(yytext[1]);
    if (radix_flag == RADIX_10) {
        BEGIN(real10);
    } else {
        BEGIN(real);
    }
  }
}

<INITIAL,naninf>{
{REAL_NANINF} {
    BEGIN(complex);
}
}

<INITIAL,ureal10,real10>{
{UREAL10} {
    tmp_number = atoll(yytext);
    BEGIN(suffix);
}
{_UREAL10_FLO} {
    tmp_number = atof(yytext);
    exactness_derivation = EXACTNESS_FLO;
    BEGIN(suffix);
}
{_UREAL10} {
   BEGIN(complex);
}
}

<INITIAL,real10>{
{REAL10} {
    tmp_number = atoll(yytext);
    BEGIN(suffix);
}

{_REAL10_FLO} {
    tmp_number = atof(yytext);
    exactness_derivation = EXACTNESS_FLO;
    BEGIN(suffix);
}

{_REAL10} {
    BEGIN(complex);
}
}

<suffix>{
{EXPONENT_MARK}{SIGN}?{DIGIT10}+ {
    long long exp = atoll(yytext+1);
    exp = (long long)pow(10, exp);
    if (exactness_derivation == EXACTNESS_FIX) {
        tmp_number = (long long)tmp_number * exp;
    } else if (exactness_derivation == EXACTNESS_FLO) {
        tmp_number = (double)tmp_number * exp;
    }
    BEGIN(mantissa_width);
}

.|\n {
    yyless(yyleng - 1); BEGIN(mantissa_width);
}
}

<mantissa_width>{
"|"{DIGIT10}+ {
    BEGIN(complex);
}
.|\n {
    yyless(yyleng-1); BEGIN(complex);
}
}


<ureal,real>{
{UREAL} {
    tmp_number = strtoll(yytext, 0, radix_flag);
    BEGIN(complex);
}
    
{_UREAL} {
    BEGIN(complex);
}
}

<real>{
{REAL} {
    tmp_number = strtoll(yytext, 0, radix_flag);
    BEGIN(complex);
}

{_REAL} {
    BEGIN(complex);
}
}

<INITIAL,complex>{
"+"{NANINF}"i" {
    BEGIN(INITIAL);
}
"-"{NANINF}"i" {
    BEGIN(INITIAL);
}
"+i" {
    imaginary_part = 1;
}
"-i" {
    imaginary_part = -1;
}
}

<complex>{
"@" {
    if (imaginary_sign != '\0') {
        yyless(yyleng - 1);
        BEGIN(INITIAL);
    }

    if (radix_flag == RADIX_10) {
        BEGIN(real10);
    } else {
        BEGIN(real);
    }
}
"+"|"-" {
    if (imaginary_sign != '\0') {
        yyless(yyleng - 1);
        BEGIN(INITIAL);
    }
    imaginary_sign = yytext[0];
    real_part = tmp_number;
    if (radix_flag == RADIX_10) {
        BEGIN(ureal10);
    } else {
        BEGIN(ureal);
    }
}

"i" {
    if (imaginary_sign == '-') {
        if (exactness_derivation == EXACTNESS_FLO) {
            imaginary_part = -(double)tmp_number;
        } else if (exactness_derivation == EXACTNESS_FIX){
            imaginary_part = -(long long)tmp_number;
        }
    } else {
        imaginary_part = tmp_number;
    }
}
  
.|\n {
    yyless(yyleng - 1);
    BEGIN(INITIAL);

    if (exactness_flag != EXACTNESS_UNKOWN) {
        exactness_derivation = exactness_flag;
    }
    
    if (imaginary_sign == '\0') {
        real_part = tmp_number;
    }
    yylval->num = make_number(exactness_derivation, real_part, imaginary_part);
    
    radix_flag = RADIX_10;
    exactness_flag = EXACTNESS_UNKOWN;
    exactness_derivation = EXACTNESS_FIX;
    real_part = 0;
    imaginary_part = 0;
    tmp_number = 0;
    imaginary_sign = '\0';
    return NUMBER;
}
}

"#T" |
"#t" { return BOOLEAN_T; }

"#F" |
"#f" { return BOOLEAN_F; }

;.*$
{WS}* { }

. { yyerror(yylloc, yyscanner, yyextra, "Mystery character %c\n", *yytext); }

<<EOF>> { return END_OF_FILE; }
%%
