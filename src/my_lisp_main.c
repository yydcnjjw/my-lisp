#include <stdarg.h>
#include <stdio.h>

#include "my_lisp.h"
#include "my_lisp.tab.h"
#include "my_lisp.lex.h"

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s, ...) {
    va_list ap;
    va_start(ap, s);

    fprintf(stderr, "%d: error: ", yyget_lineno(scanner));

    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void init_parse_data(parse_data *data) {
    if (!data) {
        printf("init parse data failure\n");
    }

    data->ast = NULL;
    data->symtab = my_malloc(NHASH * sizeof(symbol *));
}

int main(int argc, char *argv[]) {
#ifdef YYDEBUG
    /* yydebug = 1; */
#endif

    parse_data data;
    init_parse_data(&data);
    yyscan_t scanner;

    env *env = new_env();
    env_add_builtins(env, &data);

    if (yylex_init_extra(&data, &scanner)) {
        perror("init alloc failed");
        return 1;
    }

    yyset_debug(1, scanner);

    for (;;) {
        printf("> ");
        yyparse(scanner, &data);
        if (data.ast) {
            object_print(eval(data.ast, env));            
        }
        data.ast = NULL;
        printf("\n");
    }
    return 0;
}
