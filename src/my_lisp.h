#ifndef MY_LISP_H
#define MY_LISP_H

#define MY_DEBUG

#include <my-os/types.h>

#include "os.h"
#include "number.h"

typedef enum {
    T_PRIMITIVE_PROC = 0x1,
    T_COMPOUND_PROC = 0x2,
    T_PROCEDURE = T_PRIMITIVE_PROC | T_COMPOUND_PROC,
    T_NUMBER = 0x4,
    T_BOOLEAN = 0x10,
    T_SYMBOL = 0x20,
    T_STRING = 0x40,
    T_PAIR = 0x80,
    T_CHARACTER = 0x100,
    T_VECTOR = 0x200,
    T_MACRO_PROC = 0x400,
    T_NULL = 0x800,
    T_ERR = 0x8000,
} object_type;

typedef struct object_t object;

typedef struct symbol_t symbol;
struct symbol_t {
    char *name;
    symbol *hash_next;
};

typedef struct env_t env;
struct env_t {
    env *parent;
    int count;
    symbol **symbols;
    object **objects;
};

struct string_t {
    char *str_p;
    size_t len;
};
typedef struct string_t string;

struct pair_t {
    object *car;
    object *cdr;
};
typedef struct pair_t pair;

struct error_t {
    char *msg;
};

typedef struct error_t error;

struct compound_proc_t {
    object *parameters;
    object *body;
    int param_count;
    symbol *varg;
    env *env;
};
typedef struct compound_proc_t compound_proc;

struct parse_data;
typedef struct parse_data parse_data;

typedef object *primitive_proc_ptr(env *env, object *args, parse_data *data);

struct primitive_proc_t {
    primitive_proc_ptr *proc;
};

typedef struct primitive_proc_t primitive_proc;

struct macro_proc_t {
    object *literals;
    object *syntax_rules;
};

typedef struct macro_proc_t macro_proc;

struct object_t {
    object_type type;
    int ref_count;
    union {
        number *number;
        bool bool_val;
        u16 char_val;
        primitive_proc *primitive_proc;
        compound_proc *compound_proc;
        macro_proc *macro_proc;
        symbol *symbol;
        pair *pair;
        string *str;
        error *err;
    };
};

struct parse_data {
    object *ast;
    symbol **symtab;
    bool is_eof;
};

symbol *lookup(parse_data *, char *);
object *new_boolean(bool val);
object *new_symbol(symbol *s);

string *make_string(char *, size_t);
object *new_string(string *);

number *make_number(struct number_flag_t flag, const number_value_t value[4]);
number *make_number_from_full(const number_full_t *number_full);
object *new_number(number *);
object *new_character(u16 ch);

object *cons(object *car, object *cdr);
object *car(object *pair);
object *cdr(object *pair);

object *NIL;

env *new_env(void);
void free_env(env *e);

void env_add_primitives(env *, parse_data *);

#define NHASH 9997

object *eval_from_ast(object *exp, env *env, parse_data *data);
void object_print(object *o, env *);

void free_lisp(parse_data *data);

object *ref(object *o);
object *unref(object *o);

/* #define TYPE_CPY(target, source)                                               \ */
/*     assert(sizeof((target)) == sizeof((source)));                              \ */
/*     memcpy(&(target), &(source), sizeof((target))) */

/* #define TO_TYPE(val, type) (*((type *)(&(val)))) */

static inline object *object_list_entry(object *list) {
    return list->type == T_PAIR ? car(list) : list;
}

static inline bool object_list_has_next(object *list) {
    bool ret_val = list;
    unref(list);
    return ret_val;
}

static inline object *object_list_next(object *idx) {
    return idx->type == T_PAIR ? cdr(idx) : (unref(idx), NULL);
}

#define for_each_object_list_entry(o, list)                                    \
    for (object *idx = ref(list);                                              \
         !object_list_has_next(ref(idx))                                       \
             ? false                                                           \
             : (o = object_list_entry(ref(idx)), true);                        \
         unref(o), idx = object_list_next(idx))

#define for_each_object_list(list)                                             \
    for (object *idx = ref(list);                                              \
         idx && (idx->type == T_PAIR ? true : (unref(idx), false));            \
         idx = cdr(idx))

static inline object *is_error(object *o) {
    bool ret = o && o->type == T_ERR;
    if (ret) {
        return o;
    } else {
        unref(o);
        return NIL;
    }
}

#define ERROR(e) for (object *error = is_error(e); error; error = NIL)

#define ASSERT(cond, fmt, ...) (!(cond) ? new_error(fmt, ##__VA_ARGS__) : NIL)

object *assert_fun_arg_type(char *func, object *o, int i, object_type type);

#ifdef MY_OS
#define YY_INPUT(_buf, result, max_size)
/* parse_data *p_data = (parse_data *)yyget_extra(yyscanner); \ */
/* int c = p_data->buf[p_data->pos++];                                        \
 */
/* result = (c == 0) ? YY_NULL : (_buf[0] = c, 1) */

void my_error(const char *);
#define YY_FATAL_ERROR(msg) my_error(msg)
#endif

/* typedef void FILE; */
/* extern int errno; */
/* extern FILE *stdin; */
/* extern FILE *stdout; */

struct lisp_ctx_opt {
#if !defined(MY_OS)
#endif // MY_OS
};

#include "my_lisp.tab.h"

struct lisp_ctx {
    yyscan_t scanner;
    parse_data *parse_data;
    env *global_env;
};

struct lisp_ctx *make_lisp_ctx(struct lisp_ctx_opt opt);
void free_lisp_ctx(struct lisp_ctx **);

#if !defined(MY_OS)
#include <stdio.h>

int eval_from_io(struct lisp_ctx *ctx, FILE *);
bool my_lisp_is_eof(struct lisp_ctx *ctx);
#endif // !defined (MY_OS)

#endif /* MY_LISP_H */
