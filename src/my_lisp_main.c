#include <stdarg.h>
#include <stdio.h>

#include "my_lisp.h"
#include "my_lisp.tab.h"
#include "my_lisp.lex.h"

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s,
             ...) {
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
    FILE *in;
    if (argc == 2) {
        in = fopen(argv[1], "r");
        yyset_in(in, scanner);
    }

    yyset_debug(1, scanner);

    for (;;) {
        int ret = yyparse(scanner, &data);
        if (data.ast && !ret) {
            object *value = eval(data.ast, env);
            object_print(value, env);
            printf("\n");
            free_object(data.ast);
            if (value != data.ast) {
                free_object(value);                
            }
            data.ast = NULL;
        } else {
            break;
        }
    }
    yylex_destroy(scanner);
    free_env(env);
    free_lisp(&data);

    fclose(in);
    return 0;
}
