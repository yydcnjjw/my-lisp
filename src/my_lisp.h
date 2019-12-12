#ifndef MY_LISP_H
#define MY_LISP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    T_FIXNUM,
    T_FLONUM,
    T_BOOLEAN,
    T_SYMBOL,
    T_STRING,
    T_PAIR,
    T_CHARACTER,
    T_ERR
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
    env *env;
};
typedef struct compound_proc_t compound_proc;

typedef object *primitive_proc(env *env, object *args);

struct object_t {
    object_type type;
    int ref_count;
    union {
        int64_t int_val;
        long double float_val;
        bool bool_val;
        char char_val;
        primitive_proc *primitive_proc;
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

symbol *lookup(parse_data*, char *);
object *new_boolean(bool val);
object *new_fix_number(int64_t val);
object *new_symbol(symbol *s);

string *make_string(char *, size_t);
object *new_string(string *);

object *cons(object *car, object *cdr);
object *NIL;

env *new_env(void);

#define NHASH 9997

object *eval(object *exp, env *env);


void object_print(object *o);

void *my_malloc(size_t size);

#endif /* MY_LISP_H */
