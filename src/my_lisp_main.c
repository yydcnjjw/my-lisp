#include "my_lisp.h"
#include "my_lisp_io.h"
#include "my_lisp.lex.h"

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
