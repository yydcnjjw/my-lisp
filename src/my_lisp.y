%define parse.trace
%define parse.error verbose

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

%token END_OF_FILE

%type <obj> number boolean symbol string list_item datum lexeme_datum compound_datum list

%token EOL

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

list_item: datum { $$ = cons($1, NIL); }
| datum list_item { $$ = cons($1, $2); }
;

list: LP list_item RP { $$ = $2;}
| LP list_item PERIOD datum RP {
    object *o;
    for_each_list(o, $2) {
        object *next = cdr(idx);
        if (!next) {
            setcdr(idx, $4);
        }
        unref(next);
    }
    $$ = $2;
 }
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
