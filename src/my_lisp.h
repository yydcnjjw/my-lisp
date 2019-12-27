#ifndef MY_LISP_H
#define MY_LISP_H

#include <assert.h>
#include <my-os/types.h>

typedef enum {
    T_PRIMITIVE_PROC = 0x1,
    T_COMPOUND_PROC = 0x2,
    T_PROCEDURE = T_PRIMITIVE_PROC | T_COMPOUND_PROC,
    T_FIXNUM = 0x4,
    T_FLONUM = 0x8,
    T_NUMBER = T_FIXNUM | T_FLONUM,
    T_BOOLEAN = 0x10,
    T_SYMBOL = 0x20,
    T_STRING = 0x40,
    T_PAIR = 0x80,
    T_CHARACTER = 0x100,
    T_ERR = 0x200,
    T_NULL = 0x400,
    T_MACRO_PROC = 0x800,
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
        int64_t int_val;
        long double float_val;
        bool bool_val;
        char char_val;
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
};

symbol *lookup(parse_data *, char *);
object *new_boolean(bool val);
object *new_fix_number(int64_t val);
object *new_symbol(symbol *s);

string *make_string(char *, size_t);
object *new_string(string *);

object *cons(object *car, object *cdr);
object *car(object *pair);
object *cdr(object *pair);
object *setcdr(object *list, object *cdr);

object *NIL;

env *new_env(void);
void free_env(env *e);

void env_add_primitives(env *, parse_data *);

#define NHASH 9997

object *eval(object *exp, env *env, parse_data *data);
void object_print(object *o, env *);

void *my_malloc(size_t size);

void free_lisp(parse_data *data);

void eof_handle(void);

object *ref(object *o);
object *unref(object *o);

#endif /* MY_LISP_H */
