#include "my_lisp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void *my_malloc(size_t size) {
    void *ret = malloc(size);
    if (!ret) {
        printf("malloc error\n");
        exit(0);
    }
    return ret;
}

static object True = {.type = T_BOOLEAN, .bool_val = true};
static object False = {.type = T_BOOLEAN, .bool_val = false};
object *NIL = NULL;

static inline object *new_object(object_type type) {
    object *o = my_malloc(sizeof(object));
    o->type = type;
    return o;
}

object *new_boolean(bool val) { return val ? &True : &False; }

object *new_fix_number(int64_t val) {
    object *fix_number = new_object(T_FIXNUM);
    fix_number->int_val = val;
    return fix_number;
}

static unsigned symhash(char *sym) {
    unsigned int hash = 0;
    unsigned c;

    while ((c = *sym++))
        hash = hash * 9 ^ c;
    return hash;
}

symbol *lookup(parse_data *data, char *ident) {
    symbol **symtab = data->symtab;
    
    symbol **sym_p;
    symbol *sym;
    sym_p = symtab + (symhash(ident) % NHASH);

    for (;;) {
        sym = *sym_p;
        if (!sym) {
            sym = my_malloc(sizeof(symbol));
            sym->name = strdup(ident);
            sym->hash_next = NULL;
            *sym_p = sym;
            break;
        }
        if (!strcmp(sym->name, ident))
            break;
        sym_p = &(sym->hash_next);
    }
    return sym;
}

object *new_symbol(symbol *s) {
    object *symbol = new_object(T_SYMBOL);
    symbol->symbol = s;
    return symbol;
}

pair *make_pair(object *car, object *cdr) {
    pair *pair = my_malloc(sizeof(pair));
    pair->car = car;
    pair->cdr = cdr;
    return pair;
}

object *cons(object *car, object *cdr) {
    object *pair = new_object(T_PAIR);
    pair->pair = make_pair(car, cdr);
    return pair;
}

string *make_string(char *str, size_t size) {
    string *s = my_malloc(sizeof(string));
    s->str_p = my_malloc(size + 1);
    s->len = size;
    memcpy(s->str_p, str, size);
    return s;
}

object *new_string(string *s) {
    object *str = new_object(T_STRING);
    str->str = s;
    return str;
}

error *make_error(char *fmt, ...) {
    error *err = my_malloc(sizeof(error));
    va_list args;
    va_start(args, fmt);
    err->msg = my_malloc(512);
    vsnprintf(err->msg, 511, fmt, args);
    err->msg = realloc(err->msg, strlen(err->msg) + 1);
    va_end(args);
    return err;
}

object *new_error(char *fmt, ...) {
    object *err = new_object(T_ERR);
    va_list args;
    va_start(args, fmt);
    err->err = make_error(fmt, args);
    va_end(args);
    return err;
}

env *new_env(void) {
    env *e = my_malloc(sizeof(env));
    bzero(e, sizeof(env));
    return e;
}

void del_env(env *e) {
    for (int i = 0; i < e->count; i++) {
        free(e->symbols[i]);
        // TODO: object remove
        // NOTE: ref count
    }
    free(e->symbols);
    free(e->objects);
    free(e);
}

object *env_get(env *e, symbol *sym) {
    for (int i = 0; i < e->count; i++) {
        if (e->symbols[i] == sym) {
            return e->objects[i];
        }
    }

    if (e->parent) {
        return env_get(e->parent, sym);
    } else {
        return new_error("Exception: variable %s is not bound", sym->name);
    }
}

void env_put(env *e, symbol *sym, object *obj) {
    for (int i = 0; i < e->count; i++) {
        if (e->symbols[i] == sym) {
            // TODO: object remove
            // NOTE: ref count
            e->objects[i] = obj;
            return;
        }
    }

#define ENV_INC 10
    if ((e->count % ENV_INC) == 0) {
        e->symbols =
            realloc(e->symbols, sizeof(symbol *) * (ENV_INC + e->count));
        e->objects =
            realloc(e->objects, sizeof(object *) * (ENV_INC + e->count));
    }

    e->symbols[e->count] = sym;
    e->objects[e->count] = obj;

    obj->ref_count++;
    e->count++;
}

void object_print(object *);

void object_pair_print(pair *pair) {
    printf("(");
    object_print(pair->car);

    printf(" . ");

    if (pair->cdr == NIL) {
        printf("()");
        return;
    }
    
    object_print(pair->cdr);
    printf(")");
}

void object_print(object *o) {
    switch (o->type) {
    case T_PAIR:
        object_pair_print(o->pair);
        break;
    case T_SYMBOL:
        printf("%s", o->symbol->name);
        break;
    case T_STRING:
        printf("\"%s\"", o->str->str_p);
        break;
    case T_FIXNUM:
    case T_FLONUM:
        printf("%li", o->int_val);
        break;
    case T_BOOLEAN:
        o->bool_val ? printf("#t") : printf("#f");
        break;
    case T_CHARACTER:
        break;
    case T_ERR:
        printf("%s", o->err->msg);
        break;
    }
}

object *eval_pair() {
    
}

object *eval(object *exp, env *env) {
    if (!exp) {
        return NULL;
    }

    if (exp->type == T_SYMBOL) {
        return env_get(env, exp->symbol);
    }/*  else if (exp->type == T_PAIR) { */
        
    /* } */

    return exp;
}
