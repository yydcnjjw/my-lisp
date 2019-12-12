%define parse.trace
%define api.pure full

%param { yyscan_t scanner }
%parse-param { parse_data *data }

%code requires {
    typedef void* yyscan_t;
    #include "my_lisp.h"
    void yyerror(yyscan_t scanner, parse_data *data, char *s, ...);    
}

%code {
    #include "my_lisp.lex.h"
}

%union {
    int64_t fix_num;
    symbol *symbol;
    object *obj;
    string *str;
}

%token LP RP LSB RSB APOSTROPHE GRAVE COMMA COMMA_AT PERIOD
%token NS_APOSTROPHE NS_GRAVE NS_COMMA NS_COMMA_AT
%token VECTOR_LP VECTOR_BYTE_LP

%token <symbol> IDENTIFIER
%token BOOLEAN_T BOOLEAN_F
%token <fix_num> NUMBER
%token character
%token <str> STRING

%type <obj> number boolean symbol pair string list_item datum lexeme_datum compound_datum list

%token EOL

%start exp

%%
exp:
| datum EOL { data->ast = $1; YYACCEPT; }
;

datum: lexeme_datum
| compound_datum
;

lexeme_datum: boolean
| number
| symbol
| string
// | character
;

string: STRING { $$ = new_string($1); }
;

number: NUMBER { $$ = new_fix_number($1); }
;

boolean: BOOLEAN_T  { $$ = new_boolean(true); }
| BOOLEAN_F  { $$ = new_boolean(false); }
;

symbol: IDENTIFIER { $$ = new_symbol($1); }
;

compound_datum: list
                // | vector
                // | bytevector
;

pair: datum PERIOD datum { $$ = cons($1, $3); }

list_item: datum { $$ = cons($1, NIL); }
| datum list_item { $$ = cons($1, $2); }
| pair
;

list: LP list_item RP { $$= $2;}
| LSB list_item RSB { $$ = $2; }
| LP RP { $$ = NIL; }
| LSB RSB { $$ = NIL; }
// | abbreviation
;

abbreviation: abbrev_prefix datum
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
