#include "number.h"
#include "os.h"
#include <assert.h>

s64 gcd(s64 a, s64 b) {
    if (b)
        while ((a %= b) && (b %= a))
            ;
    return a + b;
}

enum exact_flag to_exact_flag(char c) {
    enum exact_flag flag = 0;
    switch (c) {
    case 'e':
    case 'E':
        flag = EXACT;
        break;
    case 'i':
    case 'I':
        flag = INEXACT;
        break;
    default:
        flag = EXACT_UNDEFINE;
        break;
    }
    return flag;
}

enum radix_flag to_radix_flag(char c) {
    enum radix_flag flag = RADIX_UNDEFINE;
    switch (c) {
    case 'b':
    case 'B':
        flag = RADIX_2;
        break;
    case 'o':
    case 'O':
        flag = RADIX_8;
        break;
    case 'd':
    case 'D':
        flag = RADIX_10;
        break;
    case 'x':
    case 'X':
        flag = RADIX_16;
        break;
    default:
        /* flag = RADIX_UNDEFINE; */
        assert(true);
        break;
    }
    return flag;
}

u8 radix_value(enum radix_flag flag) {
    assert(flag != RADIX_UNDEFINE);
    u8 v = 10;
    switch (flag) {
    case RADIX_2:
        v = 2;
        break;
    case RADIX_8:
        v = 8;
        break;
    case RADIX_10:
        v = 10;
        break;
    case RADIX_16:
        v = 16;
        break;
    default:
        assert(true);
        break;
    }
    return v;
}

void number_part_set_flo(number_part_t *part, double v, u64 width) {
    assert(part);
    part->v[0].flo_v = v;
    part->v[1].u64_v = width;
    part->type = NUMBER_PART_FLO;
}

double number_part_get_flo_value(const number_part_t *part) {
    assert(part->type == NUMBER_PART_FLO);
    return part->v[0].flo_v;
}

u64 number_part_get_flo_width(const number_part_t *part) {
    assert(part->type == NUMBER_PART_FLO);
    return part->v[1].u64_v;
}

void str_to_number_flo(number_part_t *part, char *s,
                       enum radix_flag radix_flag) {
    assert(part);
    assert(s);
    assert(radix_flag == RADIX_10);
    size_t len = strlen(s);
    char *point = strchr(s, '.');
    int i = point - s;

    number_part_set_flo(part, my_strtod(s), len - i - 1);
}

void number_part_set_exact(number_part_t *part, s64 numerator,
                           u64 denominator) {
    assert(part);
    part->v[0].s64_v = numerator;
    part->v[1].u64_v = denominator;
    part->type = NUMBER_PART_EXACT;
}

s64 number_part_get_exact_numerator(const number_part_t *part) {
    assert(part->type == NUMBER_PART_EXACT);
    return part->v[0].s64_v;
}
u64 number_part_get_exact_denominator(const number_part_t *part) {
    assert(part->type == NUMBER_PART_EXACT);
    return part->v[1].u64_v;
}

void str_to_number_exact(number_part_t *part, char *s,
                         enum radix_flag radix_flag) {
    assert(part);
    assert(s);

    char *second = strchr(s, '/') + 1;
    int len = second - s;
    char *first = my_malloc(len);
    memcpy(first, s, len - 1);

    number_part_set_exact(part, my_strtoll(first, radix_value(radix_flag)),
                          my_strtoll(second, radix_value(radix_flag)));
    my_free(first);
}

void number_part_set_zip_exact(number_part_t *part, s64 v) {
    assert(part);
    part->v[0].s64_v = v;
    part->v[1].u64_v = 1;
    part->type = NUMBER_PART_ZIP_EXACT;
}

s64 number_part_get_zip_exact_value(const number_part_t *part) {
    assert(part->type == NUMBER_PART_ZIP_EXACT);
    return part->v[0].s64_v;
}

void str_to_number_zip_exact(number_part_t *part, char *s,
                             enum radix_flag radix_flag) {
    assert(part);
    assert(s);
    number_part_set_zip_exact(part, my_strtoll(s, radix_value(radix_flag)));
}

void number_part_set_none(number_part_t *part) {
    part->type = NUMBER_PART_NONE;
}

void lex_number_populate_prefix_from_ch(char ch, number_prefix_t *prefix) {
    assert(prefix);
    enum exact_flag exact = to_exact_flag(ch);
    if (exact != EXACT_UNDEFINE) {
        prefix->exact_type = exact;
    } else {
        prefix->radix_type = to_radix_flag(ch);
    }
}

void lex_number_populate_prefix_from_str(char *text, size_t size,
                                         number_prefix_t *prefix) {
    if (!prefix) {
        return;
    }

    if (size > 0) {
        lex_number_populate_prefix_from_ch(text[1], prefix);
    }
    if (size > 2) {
        lex_number_populate_prefix_from_ch(text[3], prefix);
    }
}

void lex_number_full_init(number_full_t *number_full) {
    if (!number_full) {
        return;
    }

    bzero(number_full, sizeof(number_full_t));

    number_full->prefix.exact_type = EXACT_UNDEFINE;
    number_full->prefix.radix_type = RADIX_10;
    number_full->complex.real.type = NUMBER_PART_NONE;
    number_full->complex.imag.type = NUMBER_PART_NONE;
}

number_part_t *number_full_get_part(number_full_t *number_full,
                                    enum complex_part_t part) {
    assert(number_full);
    number_part_t *complex_part = NULL;
    if (part == COMPLEX_PART_REAL) {
        complex_part = &number_full->complex.real;
    } else if (part == COMPLEX_PART_IMAG) {
        complex_part = &number_full->complex.imag;
    }
    return complex_part;
}

void number_part_set_naninf(number_part_t *part, enum naninf_flag naninf) {
    part->v[0].u64_v = naninf;
    part->type = NUMBER_PART_NANINF;
}

enum naninf_flag number_part_get_naninf(const number_part_t *part) {
    assert(part->type == NUMBER_PART_NANINF);
    return part->v[0].u64_v;
}

void lex_number_populate_part_naninf(number_full_t *number_full,
                                     enum complex_part_t part,
                                     enum naninf_flag naninf) {
    if (!number_full) {
        return;
    }
    number_part_t *complex_part = number_full_get_part(number_full, part);
    number_part_set_naninf(complex_part, naninf);
}

void lex_number_populate_imag_part_one_i(number_full_t *number_full,
                                         char sign) {
    s64 n = 1;
    if (sign == '-') {
        n = -1;
    }
    number_part_set_zip_exact(&number_full->complex.imag, n);
}

void lex_number_populate_part_from_str(number_full_t *number_full,
                                       enum complex_part_t part, char *text,
                                       enum number_part_type type) {
    assert(type != NUMBER_PART_NANINF && type != NUMBER_PART_NONE);
    assert(text);
    if (!number_full) {
        return;
    }
    enum radix_flag radix_flag = number_full->prefix.radix_type;
    number_part_t *complex_part = number_full_get_part(number_full, part);
    switch (type) {
    case NUMBER_PART_FLO:
        str_to_number_flo(complex_part, text, radix_flag);
        break;
    case NUMBER_PART_EXACT:
        str_to_number_exact(complex_part, text, radix_flag);
        break;
    case NUMBER_PART_ZIP_EXACT:
        str_to_number_zip_exact(complex_part, text, radix_flag);
        break;
    default:
        break;
    }
}

void lex_number_calc_part_exp_from_str(number_full_t *number_full,
                                       enum complex_part_t part,
                                       char *exp_text) {
    assert(exp_text);
    if (!number_full) {
        return;
    }
    number_part_t *complex_part = number_full_get_part(number_full, part);
    assert(complex_part->type == NUMBER_PART_FLO ||
           complex_part->type == NUMBER_PART_ZIP_EXACT);

    s64 _exp = my_strtoll(exp_text, radix_value(RADIX_10));
    s64 exp = (s64)pow(10, _exp);

    // TODO: use number operate
    /* if (!parse_real10_is_flo) { */
    /*     number_tmp[0].s64_v *= exp; */
    /* } else { */
    /*     number_tmp[0].flo_v *= exp; */
    /* }       */
}


/* enum number_operate_type { */
/*     NUMBER_OPERATE_ADD, */
/*     NUMBER_OPERATE_SUB, */
/*     NUMBER_OPERATE_MUL, */
/*     NUMBER_OPERATE_DIV */
/* }; */

/* struct number_operate_t { */
/*     int (*exact_operate)(number_value_t result[2], const number_value_t
 * var1[2], */
/*                          const number_value_t var2[2], */
/*                          const enum number_operate_type type); */
/*     int (*flo_operate)(number_value_t result[2], const number_value_t
 * var1[2], */
/*                        const number_value_t var2[2], */
/*                        const enum number_operate_type type); */
/*     int (*naninf_operate)(enum naninf_flag *result, const enum naninf_flag
 * var1, */
/*                           const enum naninf_flag var2, */
/*                           const enum number_operate_type type); */
/* }; */

/* #define DIVIDE_ERR -1; */

/* // TODO: error type */
/* int number_exact_operate(number_value_t result[2], const number_value_t
 * var1[2], */
/*                          const number_value_t var2[2], */
/*                          const enum number_operate_type type) { */
/*     my_printf("%Li  %Li : %Li %Li\n", var1[0].s64_v, var1[1].s64_v, */
/*               var2[0].s64_v, var2[1].s64_v); */

/*     switch (type) { */
/*     case NUMBER_OPERATE_ADD: */
/*         result[0].s64_v = */
/*             var1[1].s64_v * var2[0].s64_v + var2[1].s64_v * var1[0].s64_v; */
/*         result[1].s64_v = var1[1].s64_v * var2[1].s64_v; */
/*         break; */
/*     case NUMBER_OPERATE_SUB: */
/*         result[0].s64_v = */
/*             var1[1].s64_v * var2[0].s64_v - var2[1].s64_v * var1[0].s64_v; */
/*         result[1].s64_v = var1[1].s64_v * var2[1].s64_v; */
/*         break; */
/*     case NUMBER_OPERATE_MUL: */
/*         result[0].s64_v = var1[0].s64_v * var2[0].s64_v; */
/*         result[1].s64_v = var1[1].s64_v * var2[1].s64_v; */
/*         break; */
/*     case NUMBER_OPERATE_DIV: */
/*         if (!var2[0].s64_v) { */
/*             return DIVIDE_ERR; */
/*         } */

/*         if (var2[0].s64_v < 0) { */
/*             result[0].s64_v = -(var1[0].s64_v * var2[1].s64_v); */
/*             result[1].s64_v = var1[1].s64_v * (-var2[0].s64_v); */
/*         } else { */
/*             result[0].s64_v = var1[0].s64_v * var2[1].s64_v; */
/*             result[1].s64_v = var1[1].s64_v * var2[0].s64_v; */
/*         } */
/*         break; */
/*     } */

/*     my_printf("%Li / %Li\n", result[0].s64_v, result[1].s64_v); */
/*     assert(result[0].s64_v && result[1].s64_v); */
/*     s64 ret_gcd = gcd(result[0].s64_v, result[1].s64_v); */
/*     if (ret_gcd != 1) { */
/*         result[0].s64_v /= ret_gcd; */
/*         result[1].s64_v /= ret_gcd; */
/*     } */
/*     return 0; */
/* } */

/* int number_flo_operate(number_value_t result[2], const number_value_t
 * var1[2], */
/*                        const number_value_t var2[2], */
/*                        const enum number_operate_type type) { */
/*     double f_var1 = var1[0].flo_v; */
/*     double f_var2 = var2[0].flo_v; */

/*     u64 w_var1 = var1[1].u64_v; */
/*     u64 w_var2 = var2[1].u64_v; */

/*     double f_result = 0; */

/*     switch (type) { */
/*     case NUMBER_OPERATE_ADD: */
/*         f_result = f_var1 + f_var2; */
/*         break; */
/*     case NUMBER_OPERATE_SUB: */
/*         f_result = f_var1 - f_var2; */
/*         break; */
/*     case NUMBER_OPERATE_MUL: */
/*         f_result = f_var1 * f_var2; */
/*         break; */
/*     case NUMBER_OPERATE_DIV: */
/*         if (f_var2 == 0.0f) { */
/*             return DIVIDE_ERR; */
/*         } */
/*         f_result = f_var1 / f_var2; */
/*         break; */
/*     } */

/*     result[0].flo_v = f_result; */
/*     result[1].u64_v = max(w_var1, w_var2); */
/*     return 0; */
/* } */

/* int number_naninf_operate(enum naninf_flag *result, const enum naninf_flag
 * var1, */
/*                           const enum naninf_flag var2, */
/*                           const enum number_operate_type type) { */
/*     if (!result) { */
/*         return 0; */
/*     } */

/*     if (!var1 && var2) { */
/*         *result = var2; */
/*         return 0; */
/*     } */
/*     if (!var2 && var1) { */
/*         *result = var1; */
/*         return 0; */
/*     } */
/*     *result = NAN_POSITIVE; */
/*     return 0; */
/* } */

/* int number_complex_operate(number_value_t result[4], */
/*                            const number_value_t var1[4], */
/*                            const number_value_t var2[4], */
/*                            const enum number_operate_type type) {} */

/* number *number_add(number *op1, number *op2) { */
/*     struct number_flag_t op1_flag = op1->flag; */
/*     struct number_flag_t op2_flag = op2->flag; */

/*     bool is_flo = op1_flag.flo || op2_flag.flo; */

/*     number_value_t op1_uzip[4] = {}; */
/*     unzip_number_value(op1_uzip, op1); */

/*     if (!op1_flag.flo && is_flo) { */
/*         unzip_number_to_flo(op1_uzip, op1_uzip); */
/*     } */

/*     number_value_t op2_uzip[4] = {}; */
/*     unzip_number_value(op2_uzip, op2); */

/*     if (!op2_flag.flo && is_flo) { */
/*         unzip_number_to_flo(op2_uzip, op2_uzip); */
/*     } */

/*     number_value_t result_value[4] = {}; */

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
/*         result_flag.exact_zip |= result_value[1].u64_v == 1 ? _REAL_BIT : 0;
 */
/*         result_flag.exact_zip |= result_value[3].u64_v == 1 ? _IMAG_BIT : 0;
 */
/*     } */
/*     number *result = make_number(result_flag, result_value); */
/*     return result; */
/* } */

/* // example: 3 => 3/1 */
/* void __number_add(number_value_t result[2], number_value_t var1[2], */
/*                   number_value_t var2[2], bool is_flo) { */
/*     if (!is_flo) { */
/*         format_exact(var1); */
/*         format_exact(var2); */
/*         my_printf("%Li  %Li : %Li %Li\n", var1[0].s64_v, var1[1].s64_v, */
/*                   var2[0].s64_v, var2[1].s64_v); */
/*         if (!var1[0].s64_v) { */
/*             memcpy(result, var2, sizeof(number_value_t) * 2); */
/*             unformat_exact(result); */
/*             return; */
/*         } */

/*         if (!var2[0].s64_v) { */
/*             memcpy(result, var1, sizeof(number_value_t) * 2); */
/*             unformat_exact(result); */
/*             return; */
/*         } */

/*         result[0].s64_v = */
/*             var1[1].s64_v * var2[0].s64_v + var2[1].s64_v * var1[0].s64_v; */
/*         result[1].s64_v = var1[1].s64_v * var2[1].s64_v; */
/*         my_printf("%Li / %Li\n", result[0].s64_v, result[1].s64_v); */
/*         assert(result[0].s64_v && result[1].s64_v); */
/*         s64 ret_gcd = gcd(result[0].s64_v, result[1].s64_v); */
/*         if (ret_gcd != 1) { */
/*             result[0].s64_v /= ret_gcd; */
/*             result[1].s64_v /= ret_gcd; */
/*         } */
/*         unformat_exact(result); */
/*     } else { */
/*         double f_var1 = var1[0].flo_v; */
/*         double f_var2 = var2[0].flo_v; */
/*         my_printf("op1 %f op2 %f\n", f_var1, f_var2); */
/*         double f_result = f_var1 + f_var2; */
/*         result[0].flo_v = f_result; */
/*     } */
/* } */

/* enum naninf_flag __number_add_naninf(enum naninf_flag var1, */
/*                                      enum naninf_flag var2) { */

/*     if (!var1 && var2) { */
/*         return var2; */
/*     } */
/*     if (!var2 && var1) { */
/*         return var1; */
/*     } */
/*     return NAN_POSITIVE; */
/* } */

/* number *cpy_number(number *source) { */
/*     size_t value_size = source->flag.size * sizeof(u64); */
/*     size_t size = sizeof(number) + value_size; */
/*     number *result = my_malloc(size); */
/*     memcpy(result, source, size); */
/*     return result; */
/* } */

/* void _cpy_to_unzip_number(number_value_t target[2], number_value_t *source,
 */
/*                           bool is_flo, bool is_exact, bool is_naninf) { */
/*     if (is_naninf) { */
/*         bzero(target, sizeof(u64) * 2); */
/*     } else if (is_exact || is_flo) { */
/*         /\* assert(source[1].s64_v); *\/ */
/*         memcpy(target, source, sizeof(number_value_t) * 2); */
/*     } else { */
/*         target[0] = source[0]; */
/*         target[1].s64_v = 1; */
/*     } */
/* } */

/* enum number_part_type number_get_part_type() {} */

/* int number_get_part_type_zip_size(const enum number_part_type type) { */
/*     switch (type) {} */
/* } */

/* void unzip_number_value(number_value_t result[4], number *source) { */
/*     bzero(result, sizeof(u64) * 4); */

/*     struct number_flag_t flag = source->flag; */
/*     number_value_t *value = source->value; */

/*     bool is_flo = flag.flo; */
/*     bool is_exact = flag.exact & _REAL_BIT; */
/*     enum naninf_flag naninf_flag = flag.naninf & 0x0f; */

/*     _cpy_to_unzip_number(result, value, is_flo, is_exact, naninf_flag); */

/*     if (!flag.complex) { */
/*         return; */
/*     } */

/*     naninf_flag = flag.naninf >> 4; */
/*     if (!naninf_flag) { */
/*         if (is_exact || is_flo) { */
/*             value += 2; */
/*         } else { */
/*             value += 1; */
/*         } */
/*     } */
/*     is_exact = flag.exact & _IMAG_BIT; */
/*     _cpy_to_unzip_number(result + 2, value, is_flo, is_exact, naninf_flag);
 */
/* } */

/* void unzip_number_to_flo(number_value_t result[4], number_value_t source[4])
 * { */
/*     if (result != source) { */
/*         memcpy(result, source, sizeof(u64) * 4); */
/*     } */

/*     _exact_to_flo(result, true); */
/*     _exact_to_flo(result + 2, true); */
/* } */

/* number *_number_add(number *op1, number *op2) { */
/*     struct number_flag_t op1_flag = op1->flag; */
/*     struct number_flag_t op2_flag = op2->flag; */

/*     bool is_flo = op1_flag.flo || op2_flag.flo; */

/*     number_value_t op1_uzip[4] = {}; */
/*     unzip_number_value(op1_uzip, op1); */

/*     if (!op1_flag.flo && is_flo) { */
/*         unzip_number_to_flo(op1_uzip, op1_uzip); */
/*     } */

/*     number_value_t op2_uzip[4] = {}; */
/*     unzip_number_value(op2_uzip, op2); */

/*     if (!op2_flag.flo && is_flo) { */
/*         unzip_number_to_flo(op2_uzip, op2_uzip); */
/*     } */

/*     number_value_t result_value[4] = {}; */

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
/*         result_flag.exact |= result_value[1].u64_v == 1 ? _REAL_BIT : 0; */
/*         result_flag.exact |= result_value[3].u64_v == 1 ? _IMAG_BIT : 0; */
/*     } */
/*     number *result = make_number(result_flag, result_value); */
/*     return result; */
/* } */

/* size_t calc_number_value_len(struct number_flag_t flag) { */
/*     int value_len = 1; */
/*     if (flag.flo) { */
/*         ++value_len; */
/*     } */
/*     if (flag.naninf & 0x0f) { */
/*         --value_len; */
/*     } */
/*     if (flag.complex) { */
/*         ++value_len; */
/*         if (flag.flo) { */
/*             ++value_len; */
/*         } */
/*         if (flag.naninf & 0xf0) { */
/*             --value_len; */
/*         } */
/*     } */

/*     value_len += (flag.exact + 1) / 2; */
/*     return value_len; */
/* } */

void number_part_to_flo(number_part_t *part) {
    switch (part->type) {
    case NUMBER_PART_EXACT: {
        s64 n1 = part->v[0].s64_v;
        u64 n2 = part->v[1].u64_v;
        double n = n2 ? (double)n1 / (double)n2 : (double)n1;
        number_part_set_flo(part, n, 10);
        break;
    }
    case NUMBER_PART_ZIP_EXACT: {
        double n = part->v[0].s64_v;
        number_part_set_flo(part, n, 0);
        break;
    }
    default:
        break;
    }
}

void number_part_to_exact(number_part_t *part) {
    switch (part->type) {
    case NUMBER_PART_FLO: {
        u64 width = part->v[1].u64_v;
        double v = part->v[0].flo_v;
        s64 denominator = pow(10, width);
        s64 numerator = v * denominator;
        s64 gcd_v = gcd(numerator, denominator);
        number_part_set_exact(part, numerator / gcd_v, denominator / gcd_v);
    }
    default:
        break;
    }
}

int lex_number_full_normalize(number_full_t *value) {
    assert(value);
    number_prefix_t *prefix = &value->prefix;
    number_part_t *real = number_full_get_part(value, COMPLEX_PART_REAL);
    number_part_t *imag = number_full_get_part(value, COMPLEX_PART_IMAG);

    bool is_flo =
        (real->type == NUMBER_PART_FLO || imag->type == NUMBER_PART_FLO) &&
        prefix->exact_type != EXACT;

    if (is_flo) {
        number_part_to_flo(real);
        number_part_to_flo(imag);
        prefix->exact_type = INEXACT;
    } else {
        number_part_to_exact(real);
        number_part_to_exact(imag);
        prefix->exact_type = EXACT;
    }
    return 0;
}

size_t number_get_part_zip_size(const number_part_t *part) {
    size_t size = 0;
    switch (part->type) {
    case NUMBER_PART_FLO:
    case NUMBER_PART_EXACT:
        size = 2;
        break;
    case NUMBER_PART_ZIP_EXACT:
        size = 1;
        break;
    case NUMBER_PART_NANINF:
    case NUMBER_PART_NONE:
        size = 0;
        break;
    }
    return size;
}

size_t number_calc_full_zip_size(const number_full_t *source) {
    size_t size = sizeof(number);
    const number_part_t *real = &source->complex.real;
    const number_part_t *imag = &source->complex.imag;

    size += number_get_part_zip_size(real) * sizeof(number_value_t);
    size += number_get_part_zip_size(imag) * sizeof(number_value_t);

    return size;
}

int number_zip_part(number_value_t *result, const number_part_t *part) {
    size_t off = 0;
    switch (part->type) {
    case NUMBER_PART_FLO:
    case NUMBER_PART_EXACT:
        result[0] = part->v[0];
        result[1] = part->v[1];
        off = 2;
        break;
    case NUMBER_PART_ZIP_EXACT:
        result[0] = part->v[0];
        off = 1;
        break;
    default:
        off = 0;
        break;
    }
    return off;
}

number *number_zip_full_number(number_full_t *source) {
    size_t size = number_calc_full_zip_size(source);
    number *num = my_malloc(size);

    number_value_t *value = num->value;
    value += number_zip_part(value, &source->complex.real);
    number_zip_part(value, &source->complex.imag);

    num->flag.complex = source->complex.imag.type != NUMBER_PART_NONE;
    num->flag.flo = source->prefix.exact_type == INEXACT;
    num->flag.radix = source->prefix.radix_type;
    num->flag.size = size;

    number_part_t *real = &source->complex.real;
    number_part_t *imag = &source->complex.imag;

    num->flag.exact_zip |=
        (real->type == NUMBER_PART_ZIP_EXACT) ? _REAL_BIT : 0;
    num->flag.exact_zip |=
        (imag->type == NUMBER_PART_ZIP_EXACT) ? _IMAG_BIT : 0;

    num->flag.exact |= (real->type == NUMBER_PART_EXACT) ? _REAL_BIT : 0;
    num->flag.exact |= (imag->type == NUMBER_PART_EXACT) ? _IMAG_BIT : 0;

    num->flag.naninf |=
        real->type == NUMBER_PART_NANINF ? number_part_get_naninf(real) : 0;
    num->flag.naninf |= imag->type == NUMBER_PART_NANINF
                            ? number_part_get_naninf(imag) << 4
                            : 0;
    return num;
}

enum naninf_flag
number_unzip_get_naninf_flag(const number *source,
                             enum complex_part_t complex_part) {
    return complex_part == COMPLEX_PART_REAL ? source->flag.naninf
                                             : source->flag.naninf >> 4;
}

void number_unzip_extract_prefix(number_prefix_t *prefix,
                                 const number *source) {
    assert(prefix);
    prefix->radix_type = source->flag.radix;
    prefix->exact_type = source->flag.flo ? INEXACT : EXACT;
}

enum number_part_type
number_unzip_get_part_type(const number *source,
                           enum complex_part_t complex_part) {
    number_flag_t flag = source->flag;
    u8 part_bit = complex_part == COMPLEX_PART_REAL ? _REAL_BIT : _IMAG_BIT;

    bool is_flo = flag.flo;
    bool is_exact_zip = (flag.exact_zip & part_bit);
    bool is_exact = (flag.exact & part_bit);
    bool is_naninf = number_unzip_get_naninf_flag(source, complex_part);

    enum number_part_type type = NUMBER_PART_NONE;
    if (is_flo) {
        type = NUMBER_PART_FLO;
    } else if (is_exact) {
        type = NUMBER_PART_EXACT;
    } else if (is_naninf) {
        type = NUMBER_PART_NANINF;
    } else if (is_exact_zip) {
        type = NUMBER_PART_ZIP_EXACT;
    }

    if (complex_part == COMPLEX_PART_IMAG && !flag.complex) {
        type = NUMBER_PART_NONE;
    }
    return type;
}

void number_unzip_extract_part(number_part_t *part, const number *source,
                               enum complex_part_t complex_part) {
    const number_value_t *value = source->value;
    enum number_part_type type =
        number_unzip_get_part_type(source, complex_part);
    switch (type) {
    case NUMBER_PART_FLO:
        number_part_set_flo(part, value[0].flo_v, value[1].u64_v);
        break;
    case NUMBER_PART_EXACT:
        number_part_set_exact(part, value[0].s64_v, value[1].u64_v);
        break;
    case NUMBER_PART_ZIP_EXACT:
        number_part_set_zip_exact(part, value[0].s64_v);
        break;
    case NUMBER_PART_NANINF:
        number_part_set_naninf(
            part, number_unzip_get_naninf_flag(source, complex_part));
        break;
    case NUMBER_PART_NONE:
        number_part_set_none(part);
        break;
    }
}

void number_unzip_number(number_full_t *number_full, const number *source) {
    if (!number_full) {
        return;
    }
    bzero(number_full, sizeof(number_full_t));

    number_unzip_extract_part(&number_full->complex.real, source,
                              COMPLEX_PART_REAL);
    number_unzip_extract_part(&number_full->complex.imag, source,
                              COMPLEX_PART_IMAG);

    if (number_full_get_part(number_full, COMPLEX_PART_REAL)->type ==
            NUMBER_PART_NONE &&
        number_full_get_part(number_full, COMPLEX_PART_IMAG)->type !=
            NUMBER_PART_NONE) {
        number_part_set_zip_exact(&number_full->complex.real, 0);
    }

    number_unzip_extract_prefix(&number_full->prefix, source);
}

number *make_number_from_full(number_full_t *number_full) {
    lex_number_full_normalize(number_full);
    return number_zip_full_number(number_full);
}

int _format_number_part_naninf(char *buf, const number_part_t *part) {
    int ret = 0;
    enum naninf_flag flag = number_part_get_naninf(part);
    switch (flag) {
    case NAN_POSITIVE:
        ret = my_sprintf(buf, "+nan.0");
        break;
    case NAN_NEGATIVE:
        ret = my_sprintf(buf, "-nan.0");
        break;
    case INF_POSITIVE:
        ret = my_sprintf(buf, "+inf.0");
        break;
    case INF_NEGATIVE:
        ret = my_sprintf(buf, "-inf.0");
        break;
    }
    return ret;
}

int _format_number_part_flo(char *buf, const number_part_t *part) {
    char fmt_buf[30] = {0};
    my_sprintf(fmt_buf, "%%+.%df", number_part_get_flo_width(part));
    return my_sprintf(buf, fmt_buf, number_part_get_flo_value(part));
}

int _format_number_part_exact(char *buf, const number_part_t *part) {
    return my_sprintf(buf, "%+Li/%Li", number_part_get_exact_numerator(part),
                      number_part_get_exact_denominator(part));
}

int _format_number_part_zip_exact(char *buf, const number_part_t *part) {
    return my_sprintf(buf, "%+Li", number_part_get_zip_exact_value(part));
}

int format_number_part(char *buf, const number_part_t *part) {
    int ret = 0;
    switch (part->type) {
    case NUMBER_PART_FLO:
        ret = _format_number_part_flo(buf, part);
        break;
    case NUMBER_PART_EXACT:
        ret = _format_number_part_exact(buf, part);
        break;
    case NUMBER_PART_ZIP_EXACT:
        ret = _format_number_part_zip_exact(buf, part);
        break;
    case NUMBER_PART_NANINF:
        ret = _format_number_part_naninf(buf, part);
        break;
    default:
        break;
    }
    return ret;
}

int format_number(char *buf, const number *number) {
    number_full_t number_full;
    number_unzip_number(&number_full, number);

    char *buf_p = buf;

    number_part_t *real = number_full_get_part(&number_full, COMPLEX_PART_REAL);
    number_part_t *imag = number_full_get_part(&number_full, COMPLEX_PART_IMAG);

    buf_p += format_number_part(buf_p, real);
    buf_p += format_number_part(buf_p, imag);
    if (imag->type != NUMBER_PART_NONE) {
        buf_p += my_sprintf(buf_p, "i");
    }
    return buf_p - buf;
}
