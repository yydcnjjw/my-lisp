#include <stdarg.h>

#include "my_lisp.h"

#include "my_lisp.lex.h"

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

int main(int argc, char *argv[]) {
#ifdef YYDEBUG
    /* yydebug = 1; */
#endif

    FILE *in;
    if (argc == 2 && (in = fopen(argv[1], "r")) != NULL) {
    } else {
        in = stdin;
    }

    struct lisp_ctx_opt opt = {};
    struct lisp_ctx *ctx = make_lisp_ctx(opt);
    eval_from_io(ctx, in);
    free_lisp_ctx(&ctx);
    return 0;
}

// TODO:
/* double pow(double x, double y) { */

/* } */
