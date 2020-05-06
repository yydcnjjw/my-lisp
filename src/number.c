#include "number.h"

s64 gcd(s64 a, s64 b) {
    if (b)
        while ((a %= b) && (b %= a))
            ;
    return a + b;
}

// value[0] is double
void _flo_to_exact(number_value_t value[2]) {
    u64 width = value[1].u64_v;
    double v = value[0].flo_v;
    s64 denominator = pow(10, width);
    s64 numerator = v * denominator;
    s64 gcd_v = gcd(numerator, denominator);
    value[0].s64_v = numerator / gcd_v;
    value[1].s64_v = denominator / gcd_v;
}

void _exact_to_flo(number_value_t value[2], bool is_exact) {
    if (is_exact) {
        s64 n1 = value[0].s64_v;
        s64 n2 = value[1].s64_v;
        double n = n2 ? (double)n1 / (double)n2 : (double)n1;
        value[0].flo_v = n;
        value[1].u64_v = 10;
    } else {
        double n = value[0].s64_v;
        value[0].flo_v = n;
        value[1].u64_v = 0;
    }
}

void format_exact(number_value_t value[2]) {
    if (!value[1].s64_v && value[0].s64_v) {
        value[1].s64_v = 1;
    }
}

void unformat_exact(number_value_t value[2]) {
    if (value[1].s64_v == 1 && value[0].s64_v) {
        value[1].s64_v = 0;
    }
}

// example: 3 => 3/1
void __number_add(number_value_t result[2], number_value_t var1[2],
                  number_value_t var2[2], bool is_flo) {
    if (!is_flo) {
        format_exact(var1);
        format_exact(var2);
        my_printf("%Li  %Li : %Li %Li\n", var1[0].s64_v, var1[1].s64_v,
                  var2[0].s64_v, var2[1].s64_v);
        if (!var1[0].s64_v) {
            memcpy(result, var2, sizeof(number_value_t) * 2);
            unformat_exact(result);
            return;
        }

        if (!var2[0].s64_v) {
            memcpy(result, var1, sizeof(number_value_t) * 2);
            unformat_exact(result);
            return;
        }

        result[0].s64_v =
            var1[1].s64_v * var2[0].s64_v + var2[1].s64_v * var1[0].s64_v;
        result[1].s64_v = var1[1].s64_v * var2[1].s64_v;
        my_printf("%Li / %Li\n", result[0].s64_v, result[1].s64_v);
        assert(result[0].s64_v && result[1].s64_v);
        s64 ret_gcd = gcd(result[0].s64_v, result[1].s64_v);
        if (ret_gcd != 1) {
            result[0].s64_v /= ret_gcd;
            result[1].s64_v /= ret_gcd;
        }
        unformat_exact(result);
    } else {
        double f_var1 = var1[0].flo_v;
        double f_var2 = var2[0].flo_v;
        my_printf("op1 %f op2 %f\n", f_var1, f_var2);
        double f_result = f_var1 + f_var2;
        result[0].flo_v = f_result;
    }
}

// example: 3 => 3/1
/* void __number_sub(u64 result[2], u64 var1[2], u64 var2[2], bool is_flo) { */
/*     if (!is_flo) { */
/*         format_exact(var1); */
/*         format_exact(var2); */
/*         s64 *i_var1 = (s64 *)var1; */
/*         s64 *i_var2 = (s64 *)var2; */
/*         s64 *i_result = (s64 *)result; */
/*         my_printf("%Li  %Li : %Li %Li\n", i_var1[0], i_var1[1], i_var2[0], */
/*                   i_var2[1]); */
/*         if (!i_var1[0]) { */
/*             memcpy(result, var2, sizeof(u64) * 2); */
/*             unformat_exact(result); */
/*             return; */
/*         } */

/*         if (!i_var2[0]) { */
/*             memcpy(result, var1, sizeof(u64) * 2); */
/*             unformat_exact(result); */
/*             return; */
/*         } */

/*         i_result[0] = i_var1[1] * i_var2[0] + i_var2[1] * i_var1[0]; */
/*         i_result[1] = i_var1[1] * i_var2[1]; */
/*         my_printf("%Li / %Li\n", i_result[0], i_result[1]); */
/*         assert(i_result[0] && i_result[1]); */
/*         s64 ret_gcd = gcd(i_result[0], i_result[1]); */
/*         if (ret_gcd != 1) { */
/*             i_result[0] /= ret_gcd; */
/*             i_result[1] /= ret_gcd; */
/*         } */
/*         unformat_exact(result); */
/*     } else { */
/*         double f_var1 = TO_TYPE(var1, double); */
/*         double f_var2 = TO_TYPE(var2, double); */
/*         double f_result = f_var1 + f_var2; */
/*         TYPE_CPY(*result, f_result); */
/*     } */
/* } */

number *cpy_number(number *source) {
    size_t value_size = source->flag.size * sizeof(u64);
    size_t size = sizeof(number) + value_size;
    number *result = my_malloc(size);
    memcpy(result, source, size);
    return result;
}

//
enum naninf_flag __number_add_naninf(enum naninf_flag var1,
                                     enum naninf_flag var2) {

    if (!var1 && var2) {
        return var2;
    }
    if (!var2 && var1) {
        return var1;
    }
    return NAN_POSITIVE;
}

void _cpy_to_unzip_number(number_value_t target[2], number_value_t *source,
                          bool is_flo, bool is_exact, bool is_naninf) {
    if (is_naninf) {
        bzero(target, sizeof(u64) * 2);
    } else {
        if (is_flo) {
            target[0] = source[0];
        } else if (is_exact) {
            assert(source[1].s64_v);
            memcpy(target, source, sizeof(number_value_t) * 2);
        } else {
            target[0] = source[0];
        }
    }
}

void unzip_number_value(number_value_t result[4], number *source) {
    bzero(result, sizeof(u64) * 4);

    struct number_flag_t flag = source->flag;
    number_value_t *value = source->value;

    bool is_flo = flag.flo;
    bool is_exact = flag.exact & _REAL_BIT;
    enum naninf_flag naninf_flag = flag.naninf & 0x0f;

    _cpy_to_unzip_number(result, value, is_flo, is_exact, naninf_flag);

    if (!flag.complex) {
        return;
    }

    naninf_flag = flag.naninf >> 4;
    if (!naninf_flag) {
        if (is_exact) {
            value += 2;
        } else {
            value += 1;
        }
    }
    is_exact = flag.exact & _IMAG_BIT;
    _cpy_to_unzip_number(result + 2, value, is_flo, is_exact, naninf_flag);
}

void unzip_number_to_flo(number_value_t result[4], number_value_t source[4]) {
    if (result != source) {
        memcpy(result, source, sizeof(u64) * 4);
    }

    _exact_to_flo(result, true);
    _exact_to_flo(result + 2, true);
}

number *_number_add(number *op1, number *op2) {
    struct number_flag_t op1_flag = op1->flag;
    struct number_flag_t op2_flag = op2->flag;

    bool is_flo = op1_flag.flo || op2_flag.flo;

    number_value_t op1_uzip[4] = {};
    unzip_number_value(op1_uzip, op1);

    if (!op1_flag.flo && is_flo) {
        unzip_number_to_flo(op1_uzip, op1_uzip);
    }

    number_value_t op2_uzip[4] = {};
    unzip_number_value(op2_uzip, op2);

    if (!op2_flag.flo && is_flo) {
        unzip_number_to_flo(op2_uzip, op2_uzip);
    }

    number_value_t result_value[4] = {};

    bool real_is_naninf = op1_flag.naninf & 0x0f || op2_flag.naninf & 0x0f;
    enum naninf_flag real_naninf = 0;
    if (!real_is_naninf) {
        __number_add(result_value, op1_uzip, op2_uzip, is_flo);
    } else {
        real_naninf =
            __number_add_naninf(op1_flag.naninf & 0x0f, op2_flag.naninf & 0x0f);
    }

    bool is_complex = op1_flag.complex || op2_flag.complex;
    enum naninf_flag imag_naninf = 0;
    if (is_complex) {
        bool imag_is_naninf = op1_flag.naninf >> 4 || op2_flag.naninf >> 4;
        if (!imag_is_naninf) {
            __number_add(result_value + 2, op1_uzip + 2, op2_uzip + 2, is_flo);
        } else {
            imag_naninf =
                __number_add_naninf(op1_flag.naninf >> 4, op2_flag.naninf >> 4);
        }
    }

    struct number_flag_t result_flag = {};
    result_flag.flo = is_flo;
    result_flag.radix =
        op1_flag.radix != op2_flag.radix ? RADIX_10 : op1_flag.radix;
    result_flag.complex = op1_flag.complex || op2_flag.complex;
    result_flag.naninf = real_naninf | imag_naninf;

    if (!is_flo) {
        result_flag.exact |= result_value[1].u64_v == 1 ? _REAL_BIT : 0;
        result_flag.exact |= result_value[3].u64_v == 1 ? _IMAG_BIT : 0;
    }
    number *result = make_number(result_flag, result_value);
    return result;
}

/* number *_number_sub(number *op1, number *op2) { */
/*     struct number_flag_t op1_flag = op1->flag; */
/*     struct number_flag_t op2_flag = op2->flag; */

/*     bool is_flo = op1_flag.flo || op2_flag.flo; */

/*     u64 op1_uzip[4] = {}; */
/*     unzip_number_value(op1_uzip, op1); */

/*     if (!op1_flag.flo && is_flo) { */
/*         unzip_number_to_flo(op1_uzip, op1_uzip); */
/*     } */

/*     u64 op2_uzip[4] = {}; */
/*     unzip_number_value(op2_uzip, op2); */

/*     if (!op2_flag.flo && is_flo) { */
/*         unzip_number_to_flo(op2_uzip, op2_uzip); */
/*     } */

/*     u64 result_value[4] = {}; */

/*     bool real_is_naninf = op1_flag.naninf & 0x0f || op2_flag.naninf & 0x0f;
 */
/*     enum naninf_flag real_naninf = 0; */
/*     if (!real_is_naninf) { */
/*         __number_add(result_value, op1_uzip, op2_uzip, is_flo); */
/*     } else { */
/*         real_naninf = */
/*             __number_add_naninf(op1_flag.naninf & 0x0f, op2_flag.naninf &
 * 0x0f); */
/*     } */

/*     bool is_complex = op1_flag.complex || op2_flag.complex; */
/*     enum naninf_flag imag_naninf = 0; */
/*     if (is_complex) { */
/*         bool imag_is_naninf = op1_flag.naninf >> 4 || op2_flag.naninf >> 4;
 */
/*         if (!imag_is_naninf) { */
/*             __number_add(result_value + 2, op1_uzip + 2, op2_uzip + 2,
 * is_flo); */
/*         } else { */
/*             imag_naninf = */
/*                 __number_add_naninf(op1_flag.naninf >> 4, op2_flag.naninf >>
 * 4); */
/*         } */
/*     } */

/*     struct number_flag_t result_flag = {}; */
/*     result_flag.flo = is_flo; */
/*     result_flag.radix = */
/*         op1_flag.radix != op2_flag.radix ? RADIX_10 : op1_flag.radix; */
/*     result_flag.complex = op1_flag.complex || op2_flag.complex; */
/*     result_flag.naninf = real_naninf | imag_naninf; */

/*     if (!is_flo) { */
/*         result_flag.exact |= result_value[1] == 1 ? _REAL_BIT : 0; */
/*         result_flag.exact |= result_value[3] == 1 ? _IMAG_BIT : 0; */
/*     } */
/*     number *result = make_number(result_flag, result_value); */
/*     return result; */
/* } */

object *primitive_op(env *e, object *args, char op, parse_data *data) {
    object *ret_val = NIL;
    char op_s[] = {op, '\0'};

    number *result = NULL;

    int i = 1;
    object *operand = NIL;

    for_each_object_list_entry(operand, args) {
        object *o = eval_from_ast(ref(operand), e, data);
        ERROR(ref(o)) {
            ret_val = error;
            goto loop_exit;
        }

        ERROR(assert_fun_arg_type(op_s, ref(o), i, T_NUMBER)) {
            ret_val = error;
            goto loop_exit;
        }

        if (!result) {
            result = cpy_number(o->number);
        } else {
            number *op1 = result;
            number *op2 = o->number;
            switch (op) {
            case '+':
                result = _number_add(op1, op2);
                break;
            case '-':
                result = _number_add(op1, op2);
                break;
            case '*':
                result = _number_add(op1, op2);
                break;
            case '/':
                result = _number_add(op1, op2);
                /* if (eval_val == 0) { */
                /*     return new_error("Division By Zero."); */
                /* } */
                break;
            }
            my_free(op1);
        }
        unref(o);
        i++;
        continue;

    loop_exit:
        unref(idx);
        unref(operand);
        unref(o);
        goto error;
    }

    ret_val = new_number(result);
    unref(args);
    return ret_val;
error:
    unref(args);
    my_free(result);
    return ret_val;
}

object *primitive_add(env *e, object *a, parse_data *data) {
    return primitive_op(e, a, '+', data);
}

object *primitive_sub(env *e, object *a, parse_data *data) {
    return primitive_op(e, a, '-', data);
}

object *primitive_mul(env *e, object *a, parse_data *data) {
    return primitive_op(e, a, '*', data);
}

object *primitive_div(env *e, object *a, parse_data *data) {
    return primitive_op(e, a, '/', data);
}

size_t calc_number_value_len(struct number_flag_t flag) {
    int value_len = 1;
    if (flag.flo) {
        ++value_len;
    }
    if (flag.naninf & 0x0f) {
        --value_len;
    }
    if (flag.complex) {
        ++value_len;
        if (flag.flo) {
            ++value_len;
        }
        if (flag.naninf & 0xf0) {
            --value_len;
        }
    }

    value_len += (flag.exact + 1) / 2;
    return value_len;
}

number *make_number(struct number_flag_t flag, const number_value_t value[4]) {
    size_t value_len = calc_number_value_len(flag);
    size_t value_size = value_len * sizeof(u64);
    number *number = my_malloc(sizeof(struct number_t) + value_size);
    number->flag = flag;
    number->flag.size = value_len;

    number_value_t *value_p = number->value;
    if (!(flag.naninf & 0x0f)) {
        *value_p++ = value[0];
        if (flag.exact == _REAL_BIT || flag.flo) {
            *value_p++ = value[1];
        }
    }

    if (!(flag.naninf & 0xf0) && flag.complex) {
        *value_p++ = value[2];
        if (flag.exact == _IMAG_BIT || flag.flo) {
            *value_p = value[3];
        }
    }

    return number;
}
