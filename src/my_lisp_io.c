#include "my_lisp_io.h"

#include "my_lisp.lex.h"

int eval_from_io(struct lisp_ctx *ctx, FILE *fi) {
    yyset_in(fi, ctx->scanner);
    while (!my_lisp_is_eof(ctx)) {
        yyparse(ctx->scanner, ctx->parse_data);
        object *value = eval_from_ast(ctx->parse_data->ast, ctx->global_env,
                                      ctx->parse_data);
        ctx->parse_data->ast = NULL;
        object_print(value, ctx->global_env);
        my_printf("\n");
    }
    fclose(fi);
    return 0;
}

bool my_lisp_is_eof(struct lisp_ctx *ctx) { return ctx->parse_data->is_eof; }

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
