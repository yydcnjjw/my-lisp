#pragma once

#include <my-os/types.h>

enum exact_flag { EXACT = 1, INEXACT, EXACT_UNDEFINE };
enum radix_flag { RADIX_2 = 0, RADIX_8, RADIX_10, RADIX_16, RADIX_UNDEFINE };
enum naninf_flag {
    NAN_POSITIVE = 1 << 0,
    INF_POSITIVE = 1 << 1,
    NAN_NEGATIVE = 1 << 2,
    INF_NEGATIVE = 1 << 3
};

typedef struct number_prefix_t {
    enum radix_flag radix_type;
    enum exact_flag exact_type;
} number_prefix_t;

enum complex_part_t { COMPLEX_PART_REAL, COMPLEX_PART_IMAG };

#define _REAL_BIT 0x1
#define _IMAG_BIT 0x2
#define _REAL_IMAG_BIT (_REAL_BIT | _IMAG_BIT)

typedef struct number_flag_t {
    u64 complex : 1;
    u64 flo : 1; /* 1 flo 0 fix */
    u64 radix : 2;
    u64 exact_zip : 2; /* 1 represent sint */
    u64 exact : 2;     /* 1 represent [+-]"uint"/"uint" */
    u64 naninf : 8;    /* low 4bit real high 4bit imag */
    u64 size : 8;      /* value size */
} number_flag_t;

typedef union number_value_t {
    u64 u64_v;
    double flo_v;
    s64 s64_v;
} number_value_t;

enum number_part_type {
    NUMBER_PART_NONE = 0, /* zip size 0 */
    /*
     * zip size 2
     * number full: part[0] double part[1] width u64
     */
    NUMBER_PART_FLO,
    /*
     * [+-]uinteger* / uinteger*
     * zip size 2
     * number full: part[0] s64 part[1] u64
     */
    NUMBER_PART_EXACT, /* zip size 2 */
    /*
     * [+-]uinteger*
     * zip size 1
     * number full: part[0] s64 part[1] 1
     */
    NUMBER_PART_ZIP_EXACT,
    /*
     * zip size 0
     * number full: part[0] enum naninf_flag
     */
    NUMBER_PART_NANINF,
};

typedef struct number_part_t {
    number_value_t v[2];
    enum number_part_type type;
} number_part_t;

typedef struct number_complex_t {
    number_part_t real;
    number_part_t imag;
} number_complex_t;

typedef struct number_full_t {
    number_prefix_t prefix;
    number_complex_t complex;
} number_full_t;

struct number_t {
    number_flag_t flag;
    number_value_t value[];
};
typedef struct number_t number;

size_t calc_number_value_len(struct number_flag_t flag);

void lex_number_full_init(number_full_t *number_full);

/**
 * @brief      number_populate_prefix_from_str
 *
 * @details    for flex parse
 *
 */
void lex_number_populate_prefix_from_str(char *text, size_t size,
                                         number_prefix_t *prefix);

void lex_number_populate_part_naninf(number_full_t *number_full,
                                     enum complex_part_t part,
                                     enum naninf_flag naninf);
void lex_number_populate_part_from_str(number_full_t *number_full,
                                       enum complex_part_t part, char *text,
                                       enum number_part_type type);

/**
 * @brief      handle {SIGN}"i"
 */
void lex_number_populate_imag_part_one_i(number_full_t *number_full, char sign);

int lex_number_calc_part_exp_from_str(number_full_t *number_full,
                                      enum complex_part_t part, char *exp_text);

int format_number(char *buf, const number *number);

#define DIVIDE_ERR -1;

enum number_part_operate_type {
    NUMBER_PART_OPERATE_ADD,
    NUMBER_PART_OPERATE_SUB,
    NUMBER_PART_OPERATE_MUL,
    NUMBER_PART_OPERATE_DIV
};

/**
 * @brief      number_operate
 *
 * @details    number operate and malloc number
 *
 */
int number_add(number **result, const number *var1, const number *var2);

int number_sub(number **result, const number *var1, const number *var2);

int number_mul(number **result, const number *var1, const number *var2);

int number_div(number **result, const number *var1, const number *var2);

number *make_number_from_full(const number_full_t *number_full);
number *make_number_real(s64 real);
number *make_number_real_flo(double real, u64 width);

number *number_cpy(number *num);

bool number_eq(number *n1, number *n2);
