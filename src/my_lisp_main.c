#include <stdarg.h>
#include <stdio.h>

#include "my_lisp.h"
#include "my_lisp.tab.h"
#include "my_lisp.lex.h"

static bool IS_EOF = false;

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s,
             ...) {
    va_list ap;
    va_start(ap, s);

    fprintf(stderr, "%d: error: ", yyget_lineno(scanner));

    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void eof_handle(void) {
    IS_EOF = true;
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
    env_add_primitives(env, &data);

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

    while (!IS_EOF) {
        int ret = yyparse(scanner, &data);
        if (!ret) {
            object* value = eval(ref(data.ast), env);
            printf("eval: ");
            object_print(data.ast, env);
            printf("\n");
            
            object_print(value, env);
            printf("\n");
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
