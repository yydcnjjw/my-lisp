#pragma once

#include <my_lisp.h>

object *primitive_add(env *e, object *a, parse_data *data);

object *primitive_sub(env *e, object *a, parse_data *data);

object *primitive_mul(env *e, object *a, parse_data *data);

object *primitive_div(env *e, object *a, parse_data *data);

size_t calc_number_value_len(struct number_flag_t flag);
number *make_number(struct number_flag_t flag, const number_value_t value[4]);
