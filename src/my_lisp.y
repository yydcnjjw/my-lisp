/* %define parse.trace */
/* %define parse.error verbose */

%define api.pure full
%locations

%param { yyscan_t scanner }
%parse-param { parse_data *data }

%code requires {
    typedef void* yyscan_t;
    #include "my_lisp.h"
}

%code {
    #include "my_lisp.lex.h"
    void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s, ...);        
}

%union {
    number *num;
    symbol *symbol;
    object *obj;
    string *str;
    u16 ch;
}

%token LP RP LSB RSB APOSTROPHE GRAVE COMMA COMMA_AT PERIOD
%token NS_APOSTROPHE NS_GRAVE NS_COMMA NS_COMMA_AT
%token VECTOR_LP VECTOR_BYTE_LP

%token <symbol> IDENTIFIER
%token BOOLEAN_T BOOLEAN_F
%token <num> NUMBER
%token <ch> CHARACTER
%token <str> STRING

%token END_OF_FILE

%type <obj> number boolean symbol string character list_item datum lexeme_datum compound_datum list abbreviation

%start exp

%%
exp: datum { data->ast = $1; YYACCEPT; }
| END_OF_FILE { eof_handle(); YYACCEPT; }
;

datum: lexeme_datum
| compound_datum
;

lexeme_datum: boolean
| number
| symbol
| string
| character
;

string: STRING { $$ = new_string($1); }
;

number: NUMBER { $$ = new_number($1); }
;

boolean: BOOLEAN_T  { $$ = new_boolean(true); }
| BOOLEAN_F  { $$ = new_boolean(false); }
;

character: CHARACTER { $$ = new_character($1); }

symbol: IDENTIFIER { $$ = new_symbol($1); }
;

compound_datum: list
                // | vector
                // | bytevector
;

list_item: datum { $$ = cons($1, NIL); }
| datum list_item { $$ = cons($1, $2); }
| datum PERIOD datum { $$ = cons($1, $3); }
;

list: LP list_item RP { $$ = $2;}
| LSB list_item RSB { $$ = $2; }
| LP RP { $$ = NIL; }
| LSB RSB { $$ = NIL; }
| abbreviation
;

abbreviation: APOSTROPHE datum {
    object *quote = new_symbol(lookup(data, "quote"));
    if ($2 == NIL) {
        $$ = cons(quote, NIL);
    } else {
        $$ = cons(quote, cons($2, NIL));
    }

 }
;

abbrev_prefix: APOSTROPHE
| GRAVE
| COMMA
| COMMA_AT
| NS_APOSTROPHE
| NS_GRAVE
| NS_COMMA
| NS_COMMA_AT
;

vector: VECTOR_LP list_item RP

bytevector: VECTOR_BYTE_LP list_item RP

%%
