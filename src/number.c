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

/*
 * number part function
 */
#define ASSERT_NUMBER_PART_TYPE(part, number_part_type)                        \
    assert(part);                                                              \
    assert((part)->type == (number_part_type))

#define ASSERT_NUMBER_PART_NOT_TYPE(part, number_part_type)                    \
    assert(part);                                                              \
    assert((part)->type != (number_part_type))

s64 number_part_get_zip_exact_value(const number_part_t *part);

void number_part_set_flo(number_part_t *part, double v, u64 width) {
    assert(part);
    part->v[0].flo_v = v;
    part->v[1].u64_v = width;
    part->type = NUMBER_PART_FLO;
}

void number_part_set_flo_zero(number_part_t *part) {
    number_part_set_flo(part, 0.0f, 0);
}

double number_part_get_flo_value(const number_part_t *part) {
    ASSERT_NUMBER_PART_TYPE(part, NUMBER_PART_FLO);
    return part->v[0].flo_v;
}

u64 number_part_get_flo_width(const number_part_t *part) {
    ASSERT_NUMBER_PART_TYPE(part, NUMBER_PART_FLO);
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

void number_part_set_exact_zero(number_part_t *part) {
    number_part_set_exact(part, 0, 1);
}

void number_part_set_exact_from_zip_exact(number_part_t *part,
                                          const number_part_t *zip_exact) {
    number_part_set_exact(part, number_part_get_zip_exact_value(zip_exact), 1);
}

s64 number_part_get_exact_numerator(const number_part_t *part) {
    ASSERT_NUMBER_PART_TYPE(part, NUMBER_PART_EXACT);
    return part->v[0].s64_v;
}
u64 number_part_get_exact_denominator(const number_part_t *part) {
    ASSERT_NUMBER_PART_TYPE(part, NUMBER_PART_EXACT);
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

void number_part_set_zip_exact_zero(number_part_t *part) {
    assert(part);
    number_part_set_zip_exact(part, 0);
}

s64 number_part_get_zip_exact_value(const number_part_t *part) {
    ASSERT_NUMBER_PART_TYPE(part, NUMBER_PART_ZIP_EXACT);
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

void number_part_set_naninf(number_part_t *part, enum naninf_flag naninf) {
    part->v[0].u64_v = naninf;
    part->type = NUMBER_PART_NANINF;
}

void number_part_set_naninf_zero(number_part_t *part) {
    number_part_set_naninf(part, NAN_POSITIVE);
}

enum naninf_flag number_part_get_naninf(const number_part_t *part) {
    ASSERT_NUMBER_PART_TYPE(part, NUMBER_PART_NANINF);
    return part->v[0].u64_v;
}

void number_part_set_zero(number_part_t *part, enum number_part_type type) {
    switch (type) {
    case NUMBER_PART_FLO:
        number_part_set_flo_zero(part);
        break;
    case NUMBER_PART_EXACT:
        number_part_set_exact_zero(part);
        break;
    case NUMBER_PART_ZIP_EXACT:
        number_part_set_zip_exact_zero(part);
        break;
    case NUMBER_PART_NANINF:
        number_part_set_naninf_zero(part);
        break;
    case NUMBER_PART_NONE:
        number_part_set_none(part);
        break;
    }
}

/**
 * @brief      number_part_to_flo
 *
 * @details    support result = part
 *
 */
void number_part_to_flo(number_part_t *result, const number_part_t *part) {
    ASSERT_NUMBER_PART_NOT_TYPE(part, NUMBER_PART_NANINF);

    assert(result);
    memcpy(result, part, sizeof(number_part_t));

    switch (part->type) {
    case NUMBER_PART_EXACT: {
        s64 num = number_part_get_exact_numerator(part);
        u64 deno = number_part_get_exact_denominator(part);
        double result_v = deno ? (double)num / (double)deno : (double)num;
        number_part_set_flo(result, result_v, 10);
        break;
    }
    case NUMBER_PART_ZIP_EXACT: {
        double result_v = number_part_get_zip_exact_value(part);
        number_part_set_flo(result, result_v, 0);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief      number_part_to_exact
 *
 * @details    support result = part
 * @param is_to_zip_exact
 *
 */
void number_part_to_exact(number_part_t *result, const number_part_t *part,
                          bool is_to_zip_exact) {
    assert(result);
    memcpy(result, part, sizeof(number_part_t));

    switch (part->type) {
    case NUMBER_PART_FLO: {
        u64 width = number_part_get_flo_width(part);
        double v = number_part_get_flo_value(part);
        s64 denominator = pow(10, width);
        s64 numerator = v * denominator;
        s64 gcd_v = gcd(numerator, denominator);
        number_part_set_exact(result, numerator / gcd_v, denominator / gcd_v);
        break;
    }
    case NUMBER_PART_ZIP_EXACT:
        if (is_to_zip_exact) {
            number_part_set_exact_from_zip_exact(result, part);
        }
        break;
    default:
        break;
    }
}

void number_part_copy(number_part_t *target, const number_part_t *source) {
    assert(target && source);
    memcpy(target, source, sizeof(number_part_t));
}

/*
 * number full function
 */
number_part_t *number_full_get_number_part(number_full_t *number_full,
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

const number_part_t *
number_full_get_number_part_const(const number_full_t *number_full,
                                  enum complex_part_t part) {
    assert(number_full);
    const number_part_t *complex_part = NULL;
    if (part == COMPLEX_PART_REAL) {
        complex_part = &number_full->complex.real;
    } else if (part == COMPLEX_PART_IMAG) {
        complex_part = &number_full->complex.imag;
    }
    return complex_part;
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

number *number_zip_full_number(const number_full_t *source) {
    size_t size = number_calc_full_zip_size(source);
    number *num = my_malloc(size);

    number_value_t *value = num->value;
    value += number_zip_part(value, &source->complex.real);
    number_zip_part(value, &source->complex.imag);

    num->flag.complex = source->complex.imag.type != NUMBER_PART_NONE;
    num->flag.flo = source->prefix.exact_type == INEXACT;
    num->flag.radix = source->prefix.radix_type;
    num->flag.size = size;

    const number_part_t *real =
        number_full_get_number_part_const(source, COMPLEX_PART_REAL);
    const number_part_t *imag =
        number_full_get_number_part_const(source, COMPLEX_PART_IMAG);

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

    /* if (number_full_get_part(number_full, COMPLEX_PART_REAL)->type == */
    /*         NUMBER_PART_NONE && */
    /*     number_full_get_part(number_full, COMPLEX_PART_IMAG)->type != */
    /*         NUMBER_PART_NONE) { */
    /*     number_part_set_zip_exact(&number_full->complex.real, 0); */
    /* } */

    number_unzip_extract_prefix(&number_full->prefix, source);
}

/*
 * format number
 */
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

    number_part_t *real =
        number_full_get_number_part(&number_full, COMPLEX_PART_REAL);
    number_part_t *imag =
        number_full_get_number_part(&number_full, COMPLEX_PART_IMAG);

    buf_p += format_number_part(buf_p, real);
    buf_p += format_number_part(buf_p, imag);
    if (imag->type != NUMBER_PART_NONE) {
        buf_p += my_sprintf(buf_p, "i");
    }
    return buf_p - buf;
}

/*
 * operate number
 */

char *format_number_part_operate_type(enum number_part_operate_type type) {
    char *s = NULL;
    switch (type) {
    case NUMBER_PART_OPERATE_ADD:
        s = "+";
        break;
    case NUMBER_PART_OPERATE_SUB:
        s = "-";
        break;

    case NUMBER_PART_OPERATE_MUL:
        s = "*";
        break;
    case NUMBER_PART_OPERATE_DIV:
        s = "/";
        break;
    default:
        s = "";
        break;
    }
    return s;
}

int number_part_exact_operate(number_part_t *result, const number_part_t *var1,
                              const number_part_t *var2,
                              const enum number_part_operate_type type) {
    s64 result_num = 0;
    u64 result_deno = 0;

    s64 var1_num = number_part_get_exact_numerator(var1);
    u64 var1_deno = number_part_get_exact_denominator(var1);

    s64 var2_num = number_part_get_exact_numerator(var2);
    u64 var2_deno = number_part_get_exact_denominator(var2);

    switch (type) {
    case NUMBER_PART_OPERATE_ADD:
        result_num = var1_num * var2_deno + var1_deno * var2_num;
        result_deno = var1_deno * var2_deno;
        break;
    case NUMBER_PART_OPERATE_SUB:
        result_num = var1_num * var2_deno - var1_deno * var2_num;
        result_deno = var1_deno * var2_deno;

        break;
    case NUMBER_PART_OPERATE_MUL:
        result_num = var1_num * var2_num;
        result_deno = var1_deno * var2_deno;
        break;
    case NUMBER_PART_OPERATE_DIV:
        if (!var2_num) {
            return DIVIDE_ERR;
        }

        if (var2_num < 0) {
            result_num = -(var1_num * var2_deno);
            result_deno = var1_deno * (-var2_num);
        } else {
            result_num = var1_num * var2_deno;
            result_deno = var1_deno * var2_num;
        }
        break;
    }
    s64 ret_gcd = gcd(result_num, result_deno);
    if (ret_gcd != 1) {
        result_num /= ret_gcd;
        result_deno /= ret_gcd;
    }

    my_printf("%Li/%Li %s %Li/%Li = %Li/%Li\n", var1_num, var1_deno,
              format_number_part_operate_type(type), var2_num, var2_deno,
              result_num, result_deno);
    number_part_set_exact(result, result_num, result_deno);
    return 0;
}

int number_part_zip_exact_operate(number_part_t *result,
                                  const number_part_t *var1,
                                  const number_part_t *var2,
                                  const enum number_part_operate_type type) {
    s64 var1_v = number_part_get_zip_exact_value(var1);
    s64 var2_v = number_part_get_zip_exact_value(var2);

    s64 result_v = 0;

    switch (type) {
    case NUMBER_PART_OPERATE_ADD:
        result_v = var1_v + var2_v;
        break;
    case NUMBER_PART_OPERATE_SUB:
        result_v = var1_v - var2_v;
        break;
    case NUMBER_PART_OPERATE_MUL:
        result_v = var1_v * var2_v;
        break;
    case NUMBER_PART_OPERATE_DIV:
        if (var2_v == 0.0f) {
            return DIVIDE_ERR;
        }
        result_v = var1_v / var2_v;
        break;
    }

    number_part_set_zip_exact(result, result_v);
    return 0;
}

int number_part_flo_operate(number_part_t *result, const number_part_t *var1,
                            const number_part_t *var2,
                            const enum number_part_operate_type type) {
    double var1_v = number_part_get_flo_value(var1);
    u64 var1_width = number_part_get_flo_width(var1);
    double var2_v = number_part_get_flo_value(var2);
    u64 var2_width = number_part_get_flo_width(var2);

    double result_v = 0;
    u64 result_width = max(var1_width, var2_width);

    switch (type) {
    case NUMBER_PART_OPERATE_ADD:
        result_v = var1_v + var2_v;
        break;
    case NUMBER_PART_OPERATE_SUB:
        result_v = var1_v - var2_v;
        break;
    case NUMBER_PART_OPERATE_MUL:
        result_v = var1_v * var2_v;
        break;
    case NUMBER_PART_OPERATE_DIV:
        if (var2_v == 0.0f) {
            return DIVIDE_ERR;
        }
        result_v = var1_v / var2_v;
        break;
    }

    number_part_set_flo(result, result_v, result_width);
    return 0;
}

int number_part_naninf_operate(number_part_t *result, const number_part_t *var1,
                               const number_part_t *var2,
                               const enum number_part_operate_type type) {
    assert(var1 && var2);

    enum naninf_flag result_naninf;

    enum naninf_flag var1_naninf =
        var1->type == NUMBER_PART_NANINF ? number_part_get_naninf(var1) : 0;
    enum naninf_flag var2_naninf =
        var2->type == NUMBER_PART_NANINF ? number_part_get_naninf(var2) : 0;
    if (!var1_naninf && var2_naninf) {
        result_naninf = var2_naninf;
        return 0;
    }
    if (!var2_naninf && var1_naninf) {
        result_naninf = var1_naninf;
        return 0;
    }
    result_naninf = NAN_POSITIVE;

    number_part_set_naninf(result, result_naninf);
    return 0;
}

// TODO: use ...
bool number_parts_has_type(enum number_part_type type,
                           const number_part_t *part1,
                           const number_part_t *part2) {
    assert(part1 && part2);
    return part1->type == type || part2->type == type;
}

int number_part_operate(number_part_t *result, const number_part_t *var1,
                        const number_part_t *var2,
                        const enum number_part_operate_type type) {

    if (!result) {
        return 0;
    }

    ASSERT_NUMBER_PART_NOT_TYPE(var1, NUMBER_PART_NONE);
    ASSERT_NUMBER_PART_NOT_TYPE(var2, NUMBER_PART_NONE);

    if (number_parts_has_type(NUMBER_PART_NANINF, var1, var2)) {
        return number_part_naninf_operate(result, var1, var2, type);
    }

    number_part_t op1;
    number_part_t op2;

    number_part_copy(&op1, var1);
    number_part_copy(&op2, var2);

    if (number_parts_has_type(NUMBER_PART_FLO, var1, var2)) {
        number_part_to_flo(&op1, var1);
        number_part_to_flo(&op2, var2);
        return number_part_flo_operate(result, &op1, &op2, type);
    }

    if (number_parts_has_type(NUMBER_PART_EXACT, var1, var2) ||
        type == NUMBER_PART_OPERATE_DIV) {
        number_part_to_exact(&op1, var1, true);
        number_part_to_exact(&op2, var2, true);
        return number_part_exact_operate(result, &op1, &op2, type);
    }
    // NOTE: fast computer for (+ - *)
    return number_part_zip_exact_operate(result, &op1, &op2, type);
}

/**
 * @brief      number_full_prefix_set_exact_type_from_complex
 *
 * @details    try populate prefix from number_full.complex
 *
 */
void number_full_prefix_set_exact_type_from_complex(number_full_t *number) {
    const number_part_t *real =
        number_full_get_number_part_const(number, COMPLEX_PART_REAL);
    const number_part_t *imag =
        number_full_get_number_part_const(number, COMPLEX_PART_IMAG);

    number->prefix.exact_type =
        number_parts_has_type(NUMBER_PART_FLO, real, imag) ? INEXACT : EXACT;
}

#define NUMBER_ERR_RET(exp)                                                    \
    do {                                                                       \
        int ecode = (exp);                                                     \
        if (ecode < 0) {                                                       \
            return ecode;                                                      \
        }                                                                      \
    } while (0)

/**
 * @brief      complex add
 *
 * @details    (a + bi) + (c + di) = (a + c) + (b + d)i
 *
 */
int number_full_complex_operate_add(number_full_t *result,
                                    const number_full_t *var1,
                                    const number_full_t *var2) {
    const number_part_t *a =
        number_full_get_number_part_const(var1, COMPLEX_PART_REAL);
    const number_part_t *b =
        number_full_get_number_part_const(var1, COMPLEX_PART_IMAG);

    const number_part_t *c =
        number_full_get_number_part_const(var2, COMPLEX_PART_REAL);
    const number_part_t *d =
        number_full_get_number_part_const(var2, COMPLEX_PART_IMAG);

    number_part_t *result_real =
        number_full_get_number_part(result, COMPLEX_PART_REAL);
    number_part_t *result_imag =
        number_full_get_number_part(result, COMPLEX_PART_IMAG);

    NUMBER_ERR_RET(
        number_part_operate(result_real, a, c, NUMBER_PART_OPERATE_ADD));

    NUMBER_ERR_RET(
        number_part_operate(result_imag, b, d, NUMBER_PART_OPERATE_ADD));

    return 0;
}

/**
 * @brief      complex sub
 *
 * @details    (a + bi) - (c + di) = (a - c) + (b - d)i
 *
 */
int number_full_complex_operate_sub(number_full_t *result,
                                    const number_full_t *var1,
                                    const number_full_t *var2) {
    const number_part_t *a =
        number_full_get_number_part_const(var1, COMPLEX_PART_REAL);
    const number_part_t *b =
        number_full_get_number_part_const(var1, COMPLEX_PART_IMAG);

    const number_part_t *c =
        number_full_get_number_part_const(var2, COMPLEX_PART_REAL);
    const number_part_t *d =
        number_full_get_number_part_const(var2, COMPLEX_PART_IMAG);

    number_part_t *result_real =
        number_full_get_number_part(result, COMPLEX_PART_REAL);
    number_part_t *result_imag =
        number_full_get_number_part(result, COMPLEX_PART_IMAG);

    NUMBER_ERR_RET(
        number_part_operate(result_real, a, c, NUMBER_PART_OPERATE_SUB));

    NUMBER_ERR_RET(
        number_part_operate(result_imag, b, d, NUMBER_PART_OPERATE_SUB));
    return 0;
}

/**
 * @brief      complex mul
 *
 * @details    (a + bi)(c + di) = (ac - bd) + (bc + ad)i
 *
 */
int number_full_complex_operate_mul(number_full_t *result,
                                    const number_full_t *var1,
                                    const number_full_t *var2) {
    const number_part_t *a =
        number_full_get_number_part_const(var1, COMPLEX_PART_REAL);
    const number_part_t *b =
        number_full_get_number_part_const(var1, COMPLEX_PART_IMAG);

    const number_part_t *c =
        number_full_get_number_part_const(var2, COMPLEX_PART_REAL);
    const number_part_t *d =
        number_full_get_number_part_const(var2, COMPLEX_PART_IMAG);

    number_part_t *result_real =
        number_full_get_number_part(result, COMPLEX_PART_REAL);
    number_part_t *result_imag =
        number_full_get_number_part(result, COMPLEX_PART_IMAG);

    number_part_t ac;
    number_part_t bd;
    NUMBER_ERR_RET(number_part_operate(&ac, a, c, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(number_part_operate(&bd, b, d, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(
        number_part_operate(result_real, &ac, &bd, NUMBER_PART_OPERATE_SUB));

    number_part_t bc;
    number_part_t ad;
    NUMBER_ERR_RET(number_part_operate(&bc, b, c, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(number_part_operate(&ad, a, d, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(
        number_part_operate(result_imag, &bc, &ad, NUMBER_PART_OPERATE_ADD));

    return 0;
}

/**
 * @brief      complex div
 *
 * @details    (a + bi)   (ac + bd)   (bc - ad)
 *             -------- = --------- + --------- i
 *             (c + di)   (c2 + d2)   (c2 + d2)
 */
int number_full_complex_operate_div(number_full_t *result,
                                    const number_full_t *var1,
                                    const number_full_t *var2) {
    const number_part_t *a =
        number_full_get_number_part_const(var1, COMPLEX_PART_REAL);
    const number_part_t *b =
        number_full_get_number_part_const(var1, COMPLEX_PART_IMAG);

    const number_part_t *c =
        number_full_get_number_part_const(var2, COMPLEX_PART_REAL);
    const number_part_t *d =
        number_full_get_number_part_const(var2, COMPLEX_PART_IMAG);

    number_part_t *result_real =
        number_full_get_number_part(result, COMPLEX_PART_REAL);
    number_part_t *result_imag =
        number_full_get_number_part(result, COMPLEX_PART_IMAG);

    number_part_t c2;
    number_part_t d2;
    number_part_t c2_add_d2;
    NUMBER_ERR_RET(number_part_operate(&c2, c, c, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(number_part_operate(&d2, d, d, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(
        number_part_operate(&c2_add_d2, &c2, &d2, NUMBER_PART_OPERATE_ADD));

    number_part_t ac;
    number_part_t bd;
    number_part_t ac_add_bd;
    NUMBER_ERR_RET(number_part_operate(&ac, a, c, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(number_part_operate(&bd, b, d, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(
        number_part_operate(&ac_add_bd, &ac, &bd, NUMBER_PART_OPERATE_ADD));

    NUMBER_ERR_RET(number_part_operate(result_real, &ac_add_bd, &c2_add_d2,
                                       NUMBER_PART_OPERATE_DIV));

    number_part_t bc;
    number_part_t ad;
    number_part_t bc_sub_ad;
    NUMBER_ERR_RET(number_part_operate(&bc, b, c, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(number_part_operate(&ad, a, d, NUMBER_PART_OPERATE_MUL));
    NUMBER_ERR_RET(
        number_part_operate(&bc_sub_ad, &bc, &ad, NUMBER_PART_OPERATE_SUB));

    NUMBER_ERR_RET(number_part_operate(result_imag, &bc_sub_ad, &c2_add_d2,
                                       NUMBER_PART_OPERATE_DIV));

    return 0;
}

int number_full_complex_operate(number_full_t *result,
                                const number_full_t *var1,
                                const number_full_t *var2,
                                const enum number_part_operate_type type) {
    ASSERT_NUMBER_PART_NOT_TYPE(
        number_full_get_number_part_const(var1, COMPLEX_PART_REAL),
        NUMBER_PART_NONE);
    ASSERT_NUMBER_PART_NOT_TYPE(
        number_full_get_number_part_const(var1, COMPLEX_PART_IMAG),
        NUMBER_PART_NONE);
    ASSERT_NUMBER_PART_NOT_TYPE(
        number_full_get_number_part_const(var2, COMPLEX_PART_REAL),
        NUMBER_PART_NONE);
    ASSERT_NUMBER_PART_NOT_TYPE(
        number_full_get_number_part_const(var2, COMPLEX_PART_IMAG),
        NUMBER_PART_NONE);

    int ret = 0;
    switch (type) {
    case NUMBER_PART_OPERATE_ADD:
        ret = number_full_complex_operate_add(result, var1, var2);
        break;
    case NUMBER_PART_OPERATE_SUB:
        ret = number_full_complex_operate_sub(result, var1, var2);
        break;
    case NUMBER_PART_OPERATE_MUL:
        ret = number_full_complex_operate_mul(result, var1, var2);
        break;
    case NUMBER_PART_OPERATE_DIV:
        ret = number_full_complex_operate_div(result, var1, var2);
        break;
    }
    return ret;
}

bool number_full_is_complex(const number_full_t *v) {
    return number_full_get_number_part_const(v, COMPLEX_PART_IMAG)->type !=
           NUMBER_PART_NONE;
}

void number_full_to_complex(number_full_t *result,
                            const number_full_t *source) {
    assert(result && source);
    memcpy(result, source, sizeof(number_full_t));

    number_part_t *real =
        number_full_get_number_part(result, COMPLEX_PART_REAL);
    number_part_t *imag =
        number_full_get_number_part(result, COMPLEX_PART_IMAG);

    if (real->type == NUMBER_PART_NONE) {
        number_part_set_zero(real, imag->type);
    } else if (imag->type == NUMBER_PART_NONE) {
        number_part_set_zero(imag, real->type);
    }
}

int number_full_operate(number_full_t *result, const number_full_t *var1,
                        const number_full_t *var2,
                        const enum number_part_operate_type type) {
    assert(result && var1 && var2);

    int ret = 0;
    bool has_complex =
        number_full_is_complex(var1) || number_full_is_complex(var2);
    if (has_complex) {
        number_full_t op1;
        number_full_t op2;
        number_full_to_complex(&op1, var1);
        number_full_to_complex(&op2, var2);
        ret = number_full_complex_operate(result, &op1, &op2, type);
    } else {
        number_part_t *real_result =
            number_full_get_number_part(result, COMPLEX_PART_REAL);

        ret = number_part_operate(
            real_result,
            number_full_get_number_part_const(var1, COMPLEX_PART_REAL),
            number_full_get_number_part_const(var2, COMPLEX_PART_REAL), type);
    }
    NUMBER_ERR_RET(ret);

    number_full_prefix_set_exact_type_from_complex(result);
    result->prefix.radix_type = RADIX_10;
    return 0;
}

number *number_cpy(number *num) {
    assert(num);
    size_t size = num->flag.size;
    number *new = my_malloc(size);
    memcpy(new, num, size);
    return new;
}

int number_operate(number **result, const number *var1, const number *var2,
                   const enum number_part_operate_type type) {
    assert(result);
    *result = NULL;

    number_full_t op1;
    number_full_t op2;
    number_unzip_number(&op1, var1);
    number_unzip_number(&op2, var2);

    number_full_t full_result;

    NUMBER_ERR_RET(number_full_operate(&full_result, &op1, &op2, type));
    *result = number_zip_full_number(&full_result);
    return 0;
}

int number_add(number **result, const number *var1, const number *var2) {
    return number_operate(result, var1, var2, NUMBER_PART_OPERATE_ADD);
}
int number_sub(number **result, const number *var1, const number *var2) {
    return number_operate(result, var1, var2, NUMBER_PART_OPERATE_SUB);
}
int number_mul(number **result, const number *var1, const number *var2) {
    return number_operate(result, var1, var2, NUMBER_PART_OPERATE_MUL);
}
int number_div(number **result, const number *var1, const number *var2) {
    return number_operate(result, var1, var2, NUMBER_PART_OPERATE_DIV);
}

/*
 * lex function
 */

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

void lex_number_populate_part_naninf(number_full_t *number_full,
                                     enum complex_part_t part,
                                     enum naninf_flag naninf) {
    if (!number_full) {
        return;
    }
    number_part_t *complex_part =
        number_full_get_number_part(number_full, part);
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
    number_part_t *complex_part =
        number_full_get_number_part(number_full, part);
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

int lex_number_calc_part_exp_from_str(number_full_t *number_full,
                                      enum complex_part_t part,
                                      char *exp_text) {
    assert(exp_text);
    if (!number_full) {
        return 0;
    }
    number_part_t *complex_part =
        number_full_get_number_part(number_full, part);
    assert(complex_part->type == NUMBER_PART_FLO ||
           complex_part->type == NUMBER_PART_ZIP_EXACT);

    s64 _exp = my_strtoll(exp_text, radix_value(RADIX_10));
    s64 exp = (s64)pow(10, _exp);
    number_part_t number_part_exp;
    number_part_set_zip_exact(&number_part_exp, exp);

    number_part_t result;
    if (number_part_operate(&result, complex_part, &number_part_exp,
                            NUMBER_PART_OPERATE_MUL) < 0) {
        return -1;
    }
    number_part_copy(complex_part, &result);
    return 0;
}

int lex_number_full_normalize(number_full_t *result,
                              const number_full_t *value) {
    assert(value && result);
    memcpy(result, value, sizeof(number_full_t));

    const number_prefix_t *prefix = &value->prefix;
    const number_part_t *real =
        number_full_get_number_part_const(value, COMPLEX_PART_REAL);
    const number_part_t *imag =
        number_full_get_number_part_const(value, COMPLEX_PART_IMAG);

    number_part_t *result_real =
        number_full_get_number_part(result, COMPLEX_PART_REAL);
    number_part_t *result_imag =
        number_full_get_number_part(result, COMPLEX_PART_IMAG);
    bool is_flo =
        number_parts_has_type(NUMBER_PART_FLO, result_real, result_imag) &&
        prefix->exact_type != EXACT;

    if (is_flo) {
        number_part_to_flo(result_real, real);
        number_part_to_flo(result_imag, imag);
        result->prefix.exact_type = INEXACT;
    } else {
        number_part_to_exact(result_real, real, false);
        number_part_to_exact(result_imag, imag, false);
        result->prefix.exact_type = EXACT;
    }
    return 0;
}

number *make_number_from_full(const number_full_t *number_full) {
    number_full_t number_full_normalize;
    lex_number_full_normalize(&number_full_normalize, number_full);
    return number_zip_full_number(&number_full_normalize);
}
