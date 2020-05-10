#pragma once

#define MY_LISP_IO
#ifdef MY_LISP_IO

#include <stdio.h>

#include "my_lisp.h"

int eval_from_io(struct lisp_ctx *ctx, FILE *);
bool my_lisp_is_eof(struct lisp_ctx *ctx);

#endif // MY_LISP_IO
