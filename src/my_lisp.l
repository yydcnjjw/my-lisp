%option noyywrap nodefault 8bit
%option yylineno
%option reentrant bison-bridge
/* %option debug */
%option header-file="my_lisp.lex.h"

%{
#include "my_lisp.tab.h"
#include "my_lisp.h"
%}
     
SPECIAL_INITIAL [!$%&*/:<=>?^_~]
LETTER [a-zA-Z]
INITIAL {SPECIAL_INITIAL}|{LETTER}
DIGIT [0-9]
SPECIAL_SUBSEQUENT [+|-|.|@]
SUBSEQUENT {INITIAL}|{DIGIT}|{SPECIAL_SUBSEQUENT}
IDENTIFIER {INITIAL}{SUBSEQUENT}*

%x string

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

{IDENTIFIER} { yylval->symbol = lookup(yyextra, yytext); return IDENTIFIER; }

{DIGIT}+"."{DIGIT}* |
"."?{DIGIT}+ { yylval->fix_num = atoll(yytext); return NUMBER; }

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

<<EOF>> { yyerror(yyscanner, yyextra, "the string misses \" to terminate before EOF"); }
[^\\\"]+ { char *yptr = yytext; while (*yptr) *string_buf_ptr++ = *yptr++;  }
\" {
    BEGIN(INITIAL);
    yylval->str = make_string(string_buf, string_buf_ptr - string_buf);
    return STRING;
}
. ;
}


"#T" |
"#t" { return BOOLEAN_T; }

"#F" |
"#f" { return BOOLEAN_F; }

\n { return EOL; }

[ \t] { }

. { yyerror(yyscanner, yyextra, "Mystery character %c\n", *yytext); }

<<EOF>> { exit(0); }
%%
