%option noyyalloc noyyrealloc noyyfree
%option noyywrap nodefault 8bit
%option yylineno
%option reentrant bison-bridge bison-locations
%option never-interactive
/* %option debug */
%option header-file="my_lisp.lex.h"

%{
#include "my_lisp.tab.h"
#include "my_lisp.h"
    void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s, ...);
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

%x string character
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
    s64 n = atoll(yytext);
    TYPE_CPY(&tmp_number, &n);
    BEGIN(suffix);
}
{_UREAL10_FLO} {
    double n = atof(yytext);
    TYPE_CPY(&tmp_number, &n);
    exactness_derivation = EXACTNESS_FLO;
    BEGIN(suffix);
}
{_UREAL10} {
   BEGIN(complex);
}
}

<INITIAL,real10>{
{REAL10} {
    s64 n = atoll(yytext);
    TYPE_CPY(&tmp_number, &n);    
    BEGIN(suffix);
}

{_REAL10_FLO} {
    double n = atof(yytext);
    TYPE_CPY(&tmp_number, &n);    
    exactness_derivation = EXACTNESS_FLO;
    BEGIN(suffix);
}

{_REAL10} {
    BEGIN(complex);
}
}

<suffix>{
{EXPONENT_MARK}{SIGN}?{DIGIT10}+ {
    s64 exp = atoll(yytext+1);
    exp = (s64)pow(10, exp);
    if (exactness_derivation == EXACTNESS_FIX) {
        s64 n = (*(s64 *)&tmp_number) * exp;
        TYPE_CPY(&tmp_number, &n);
    } else if (exactness_derivation == EXACTNESS_FLO) {
        double n = (*(double*)&tmp_number) * exp;
        TYPE_CPY(&tmp_number, &n);
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
    s64 n = strtoll(yytext, 0, radix_flag);
    TYPE_CPY(&tmp_number, &n);
    BEGIN(complex);
}
    
{_UREAL} {
    BEGIN(complex);
}
}

<real>{
{REAL} {
    s64 n = strtoll(yytext, 0, radix_flag);
    TYPE_CPY(&tmp_number, &n);
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
    s64 n = 1;
    TYPE_CPY(&imaginary_part, &n);
}
"-i" {
    s64 n = -1;
    TYPE_CPY(&imaginary_part, &n);
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
    TYPE_CPY(&real_part, &tmp_number);
    if (radix_flag == RADIX_10) {
        BEGIN(ureal10);
    } else {
        BEGIN(ureal);
    }
}

"i" {
    double n;
    if (imaginary_sign == '-') {
        if (exactness_derivation == EXACTNESS_FLO) {
            double n = -TO_TYPE(tmp_number, double);
            TYPE_CPY(&imaginary_part, &n);
        } else if (exactness_derivation == EXACTNESS_FIX){
            s64 n = -TO_TYPE(tmp_number, s64);
            TYPE_CPY(&imaginary_part, &n);
        }
    } else {
        TYPE_CPY(&imaginary_part, &tmp_number);
    }
}
  
.|\n {
    yyless(yyleng - 1);
    BEGIN(INITIAL);
    
    if (imaginary_sign == '\0') {
        TYPE_CPY(&real_part, &tmp_number);
    }
    if (exactness_flag == EXACTNESS_FLO &&
        exactness_derivation == EXACTNESS_FIX) {
        double n = (double)TO_TYPE(real_part, s64);
        TYPE_CPY(&real_part, &n);
        n = (double)TO_TYPE(imaginary_part, s64);
        TYPE_CPY(&imaginary_part, &n);
        exactness_derivation = EXACTNESS_FLO;
    } else if (exactness_flag == EXACTNESS_FIX &&
               exactness_derivation == EXACTNESS_FLO){
        s64 n = (s64)TO_TYPE(real_part, double);
        TYPE_CPY(&real_part, &n);
        n = (s64)TO_TYPE(imaginary_part, double);
        TYPE_CPY(&imaginary_part, &n);
        exactness_derivation = EXACTNESS_FIX;
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

"#\\" { BEGIN(character); }
<character>{
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
