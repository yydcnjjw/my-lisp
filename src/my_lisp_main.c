#include <stdarg.h>

#include "my_lisp.h"
#include "my_lisp.tab.h"
#include "my_lisp.lex.h"

static bool IS_EOF = false;

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, parse_data *data, const char *s,
             ...) {
    va_list ap;
    va_start(ap, s);

    my_printf("%d: error: ", yyget_lineno(scanner));

    my_printf(s, ap);
    my_printf("\n");
    va_end(ap);
}

void my_error(const char *msg) { my_printf(msg); }

void eof_handle(void) { IS_EOF = true; }

void init_parse_data(parse_data *data) {
    if (!data) {
        my_printf("init parse data failure\n");
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
        my_printf("init alloc failed");
        return 1;
    }

#ifdef MY_OS
    const char *str = "(define x 1)\n(define y 1)";
    int len = strlen(str);
    char *buf = my_malloc(len + 1);
    memcpy(buf, str, len);
    YY_BUFFER_STATE my_string_buffer = yy_scan_string(buf, scanner);
    yy_switch_to_buffer(my_string_buffer, scanner);
#else
    FILE *in;
    if (argc == 2 && (in = fopen(argv[1], "r")) != NULL) {
        yyset_in(in, scanner);
    } else {
        in = stdin;
        yyset_in(stdin, scanner);
    }
#endif // MY_OS

    yyset_debug(1, scanner);
    while (!IS_EOF) {
        int ret = yyparse(scanner, &data);
        if (!ret) {
            my_printf("eval: ");
            object_print(ref(data.ast), env);
            my_printf("\n");

            object *value = eval(data.ast, env, &data);
            object_print(value, env);
            my_printf("\n");
            data.ast = NULL;
        } else {
            break;
        }
    }

#ifdef MY_OS
    yy_delete_buffer(my_string_buffer, scanner);
    my_free(buf);
#else
    fclose(in);
#endif // MY_OS

    yylex_destroy(scanner);
    free_env(env);
    free_lisp(&data);
    return 0;
}

// TODO:
/* double pow(double x, double y) { */

/* } */
