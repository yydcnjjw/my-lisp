#ifndef MY_LISP_H
#define MY_LISP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    T_PRIMITIVE_PROC = 0x1,
    T_COMPOUND_PROC = 0x2,
    T_FIXNUM = 0x4,
    T_FLONUM = 0x8,
    T_BOOLEAN = 0x10,
    T_SYMBOL = 0x20,
    T_STRING = 0x40,
    T_PAIR = 0x80,
    T_CHARACTER = 0x100,
    T_ERR = 0x200
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

struct procedure_t {
    char *name;
};

typedef struct procedure_t procedure;

struct compound_proc_t {
    object *parameters;
    object *body;
    env *env;
    procedure procedure;
};
typedef struct compound_proc_t compound_proc;

typedef object *
primitive_proc_ptr(env *env, object *args);

struct primitive_proc_t {
    primitive_proc_ptr *proc;
    procedure procedure;
};

typedef struct primitive_proc_t primitive_proc;

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
typedef struct parse_data parse_data;

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
void env_add_builtins(env *, parse_data *);

#define NHASH 9997

object *eval(object *exp, env *env);

void del_object(object *o);
void object_print(object *o);

void *my_malloc(size_t size);

#define for_each_list(o, list)                                                 \
    for (object *idx = list;                                                   \
         ((o) = (!(idx) ? NULL                                                 \
                        : ((idx) && (idx)->type == T_PAIR) ? car((idx))        \
                                                           : idx)) != NULL;    \
         (idx) = (idx)->type == T_PAIR ? cdr((idx)) : NULL)


#endif /* MY_LISP_H */
