#include "my_lisp.h"
#include <my_lisp.tab.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *my_malloc(size_t size) {
    void *ret = malloc(size);
    if (!ret) {
        printf("malloc error\n");
        exit(0);
    }
    bzero(ret, size);
    return ret;
}

void my_free(void *o) {
    if (o) {
        free(o);
    }
}

void *my_realloc(void *p, size_t size) {
    if (!p) {
        return my_malloc(size);
    }
    return realloc(p, size);
}

char *my_strdup(char *s) {
    return strdup(s);
}

static object True = {.type = T_BOOLEAN, .bool_val = true};
static object False = {.type = T_BOOLEAN, .bool_val = false};
object *NIL = NULL;

object *new_error(char *fmt, ...);
char *to_string(object *o, ...);
const char *object_type_name(object_type type);
const char *type_name(object *o);
void free_object(object *o);

object *assert_fun_arg_type(char *func, object *o, int i, object_type type);

#define ERR_RET(e)                                                             \
    do {                                                                       \
        object *err = e;                                                       \
        if (err) {                                                             \
            return err;                                                        \
        }                                                                      \
    } while (0)

#define ERR_GOTO(e)                                                            \
    do {                                                                       \
        object *err = e;                                                       \
        if (err) {                                                             \
            goto error;                                                        \
        }                                                                      \
    } while (0)



object *ASSERT(bool cond, char *fmt, ...) {
    if (!cond) {
        va_list args;
        va_start(args, fmt);
        object *err = new_error(fmt, args);
        va_end(args);
        return err;
    }
    return NULL;
}

object *assert_fun_arg_type(char *func, object *o, int i, object_type type) {
    if (o && !(o->type & type)) {
        char *value = to_string(o);
        object *err = new_error("Function '%s' passed incorrect type for "
                                "argument %i. Got %s, Expected %s.",
                                func, to_string(o), i, object_type_name(type));
        my_free(value);
        return err;
    }
    return NULL;
}

object *ref(object *o) {
    o->ref_count++;
    return o;
}
object *unref(object *o) {
    if (o && !(--o->ref_count)) {
        free_object(o);
        o = NIL;
    }
    return o;
}

static inline object *new_object(object_type type) {
    object *o = my_malloc(sizeof(object));
    o->type = type;
    o->ref_count = 0;
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
            sym->name = my_strdup(ident);
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
    pair *p = my_malloc(sizeof(pair));
    p->car = car;
    p->cdr = cdr;
    return p;
}

object *cons(object *car, object *cdr) {
    object *pair = new_object(T_PAIR);
    pair->pair = make_pair(car, cdr);
    return pair;
}

object *car(object *list) {
    ERR_RET(assert_fun_arg_type("car", list, 0, T_PAIR));
    return list->pair->car;
}

object *setcar(object *list, object *car) {
    ERR_RET(assert_fun_arg_type("setcar", list, 0, T_PAIR));
    list->pair->car = car;
    return NIL;
}

object *cdr(object *list) {
    ERR_RET(assert_fun_arg_type("cdr", list, 0, T_PAIR));
    return list->pair->cdr;
}

object *setcdr(object *list, object *cdr) {
    ERR_RET(assert_fun_arg_type("setcdr", list, 0, T_PAIR));
    list->pair->cdr = cdr;
    return NIL;
}

void free_pair(object *o) {
    free_object(o->pair->car);
    free_object(o->pair->cdr);
    my_free(o->pair);
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

void free_string(object *s) {
    my_free(s->str->str_p);
    my_free(s->str);
}

error *make_error(char *fmt, ...) {
    error *err = my_malloc(sizeof(error));
    va_list args;
    va_start(args, fmt);
    err->msg = my_malloc(512);
    vsnprintf(err->msg, 511, fmt, args);
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

void free_error(object *e) {
    my_free(e->err->msg);
    my_free(e->err);
}

object *new_primitive_proc(primitive_proc_ptr *proc) {
    object *o = new_object(T_PRIMITIVE_PROC);
    primitive_proc *primitive = my_malloc(sizeof(primitive_proc));
    o->primitive_proc = primitive;
    primitive->proc = proc;
    return o;
}

void free_primitive_proc(object *o) { my_free(o->primitive_proc); }

object *new_compound_proc(env *env, object *params, object *body) {
    object *o = new_object(T_COMPOUND_PROC);
    compound_proc *proc = my_malloc(sizeof(compound_proc));
    proc->parameters = params;
    proc->body = body;
    proc->env = env;
    o->compound_proc = proc;
    return o;
}

void free_compound_proc(object *o) {
    compound_proc *proc = o->compound_proc;
    unref(proc->parameters);
    unref(proc->body);
    free_env(proc->env);
    my_free(proc);
}

env *new_env(void) { return my_malloc(sizeof(env)); }

void free_env(env *e) {
    for (int i = 0; i < e->count; i++) {
        unref(e->objects[i]);
    }
    my_free(e->symbols);
    my_free(e->objects);
    my_free(e);
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

symbol *env_get_sym(env *e, object *o) {
    for (int i = 0; i < e->count; i++) {
        if (e->objects[i] == o) {
            return e->symbols[i];
        }
    }

    if (e->parent) {
        return env_get_sym(e->parent, o);
    } else {
        return NULL;
    }
}

void env_put(env *e, symbol *sym, object *obj) {
    for (int i = 0; i < e->count; i++) {
        if (e->symbols[i] == sym) {
            unref(e->objects[i]);
            e->objects[i] = ref(obj);
            return;
        }
    }

#define ENV_INC 10
    if ((e->count % ENV_INC) == 0) {
        e->symbols =
            my_realloc(e->symbols, sizeof(symbol *) * (ENV_INC + e->count));
        e->objects =
            my_realloc(e->objects, sizeof(object *) * (ENV_INC + e->count));
    }

    e->symbols[e->count] = sym;
    e->objects[e->count] = ref(obj);
    e->count++;
}

void free_object(object *o) {
    if (!o || o->ref_count) {
        return;
    }

    switch (o->type) {
    case T_ERR:
        free_error(o);
        break;
    case T_STRING:
        free_string(o);
        break;
    case T_PAIR:
        free_pair(o);
        break;
    case T_PRIMITIVE_PROC:
        free_primitive_proc(o);
        break;
    case T_COMPOUND_PROC:
        free_compound_proc(o);
    case T_SYMBOL:
        break;
    default:
        break;
    }

    my_free(o);
}

char *pair_to_string(object *pair) {
    int len = 2;
    char *pair_str = my_malloc(len);

    strcat(pair_str, "(");
    object *o;
    for_each_list(o, pair) {
        char *s = to_string(o);
        int inc_len = 0;
        object *next = idx->type == T_PAIR ? cdr(idx) : NULL;
        char *cat_s = "";
        if (next) {
            if (next->type == T_PAIR) {
                inc_len = 1;
                cat_s = " ";
            } else {
                inc_len = 3;
                cat_s = " . ";
            }
        }

        len += strlen(s) + inc_len;
        pair_str = my_realloc(pair_str, len);
        strcat(pair_str, s);
        strcat(pair_str, cat_s);
        my_free(s);
    }
    strcat(pair_str, ")");
    return pair_str;
}

char *to_string(object *o, ...) {

    va_list args;
    va_start(args, o);
    env *e = va_arg(args, env *);
    va_end(args);

    if (!o) {
        return my_strdup("");
    }
#define BUF_SIZE 4096
    char buf[BUF_SIZE] = {'\0'};

    int len = 1;
    char *fmt = "%s";
    char *o_str = NULL;

    switch (o->type) {
    case T_PAIR:
        o_str = pair_to_string(o);
        len += strlen(o_str);
        break;
    case T_SYMBOL: {
        o_str = o->symbol->name;
        len += strlen(o_str);
        break;
    }
    case T_STRING: {
        fmt = "\"%s\"";
        len += o->str->len + 2;
        o_str = o->str->str_p;
        break;
    }
    case T_FIXNUM:
    case T_FLONUM: {
        len += sprintf(buf, "%li", o->int_val);
        o_str = buf;
        break;
    }
    case T_BOOLEAN:
        len += 2;
        o_str = o->bool_val ? "#t" : "#f";
        break;
    case T_CHARACTER:
        break;
    case T_ERR:
        len += strlen(o->err->msg);
        o_str = o->err->msg;
        break;
    case T_PRIMITIVE_PROC:
    case T_COMPOUND_PROC:
        fmt = "#<procedure %s>";
        symbol *symbol = env_get_sym(e, o);
        if (symbol) {
            o_str = symbol->name;
            len += strlen(o_str) + strlen(fmt) - 2;
        }
        break;
    default:
        break;
    }

    char *str = my_malloc(len);
    if (o_str) {
        snprintf(str, len, fmt, o_str);
        if (o->type == T_PAIR) {
            my_free(o_str);
        }
    }

    return str;
}

void object_print(object *o, env *e) {
    char *s = to_string(o, e);
    printf("%s", s);
    my_free(s);
}

int list_len(object *list) {
    assert(list && list->type == T_PAIR);

    int i = 0;
    object *o;
    for_each_list(o, list) { i++; }
    return i;
}

object *proc_call(env *e, object *func, object *args) {
    assert(func && func->type & (T_PRIMITIVE_PROC | T_COMPOUND_PROC));
    if (func->type == T_PRIMITIVE_PROC) {
        return func->primitive_proc->proc(e, args);
    }

    int given = list_len(args);
    int total = list_len(func->compound_proc->parameters);

    if (given != total) {
        
    }
    return new_error("compound proc no impl", T_COMPOUND_PROC);
}

object *eval_list(object *expr, env *env) {
    /* object *o; */
    /* object *eval_expr = NIL; */
    // 评估 顺序
    /* object *pair_ptr = eval_expr; */
    /* for_each_list(o, expr) { */
    /*     object *value = eval(o, env); */
    /*     if (value->type == T_ERR) { */
    /*         if (eval_expr != NIL) { */
    /*             free_object(eval_expr); */
    /*         } */
    /*         return value; */
    /*     } */

    /*     object *pair = cons(value, NIL); */
    /*     if (!eval_expr) { */
    /*         eval_expr = pair_ptr = pair; */
    /*     } else { */
    /*         // TODO: set cdr */
    /*         pair_ptr->pair->cdr = pair; */
    /*         pair_ptr = pair; */
    /*     } */
    /* } */

    object *operator= eval(car(expr), env);
    
    if (operator&& !(operator->type &(T_PRIMITIVE_PROC | T_COMPOUND_PROC))) {
        char *s = to_string(operator);
        object *e =
            new_error("Exception: attempt to apply non-procedure %s", s);
        my_free(s);
        return e;
    }

    object *operands = cdr(expr);
    object *ret = proc_call(env, operator, operands);
    return ret;
}

object *eval(object *exp, env *env) {
    if (!exp) {
        return NULL;
    }

    object *result = exp;
    if (exp->type == T_SYMBOL) {
        result = env_get(env, exp->symbol);
    } else if (exp->type == T_PAIR) {
        result = eval_list(exp, env);
    }
    return result;
}

void env_add_builtin(parse_data *data, env *env, char *name,
                     primitive_proc_ptr *proc) {
    env_put(env, lookup(data, name), new_primitive_proc(proc));
}

const char *object_type_name(object_type type) {
    switch (type) {
    case T_PRIMITIVE_PROC:
    case T_COMPOUND_PROC:
        return "procedure";
    case T_FIXNUM:
    case T_FLONUM:
        return "number";
    case T_BOOLEAN:
        return "boolean";
    case T_SYMBOL:
        return "symbol";
    case T_PAIR:
        return "pair";
    case T_CHARACTER:
        return "character";
    case T_ERR:
        return "error";
    default:
        return "unknown";
    }
}

const char *type_name(object *o) {
    if (o == NIL) {
        return "()";
    }
    return object_type_name(o->type);
}

object *builtin_op(env *e, object *args, char op) {
    object *first = eval(car(args), e);
    /* setcar(args, first); */
    ERR_RET(assert_fun_arg_type(&op, first, 0, T_FIXNUM | T_FLONUM));
    int value = first->int_val;

    object *operand;
    int i = 1;
    for_each_list(operand, cdr(args)) {
        object *o = eval(operand, e);
        /* setcar(idx, o); */
        ERR_RET(assert_fun_arg_type(&op, o, i, T_FIXNUM | T_FLONUM));
        int eval_val = o->int_val;
        switch (op) {
        case '+':
            value += eval_val;
            break;
        case '-':
            value -= eval_val;
            break;
        case '*':
            value *= eval_val;
            break;
        case '/':
            if (eval_val == 0) {
                return new_error("Division By Zero.");
            }
            value /= eval_val;
            break;
        }

        i++;
    }

    return new_fix_number(value);
}

object *builtin_add(env *e, object *a) { return builtin_op(e, a, '+'); }
object *builtin_sub(env *e, object *a) { return builtin_op(e, a, '-'); }
object *builtin_mul(env *e, object *a) { return builtin_op(e, a, '*'); }
object *builtin_div(env *e, object *a) { return builtin_op(e, a, '/'); }

object *builtin_define(env *e, object *args) {
    object *variable = car(args);
    object *value = NIL;
    if (variable->type == T_SYMBOL) {
        value = car(cdr(args));
    } else {
        variable = car(car(args));
        if (variable->type != T_SYMBOL) {
            return new_error("invalid syntax");
        }
        object *params = cdr(car(args));
        object *arg;
        for_each_list(arg, params) {
            ERR_RET(ASSERT(arg->type == T_SYMBOL, "invalid syntax"));
        }

        ref(params);
        object *body = ref(cdr(args));

        env *env = new_env();
        env->parent = e;
        value = new_compound_proc(env, params, body);
    }

    env_put(e, variable->symbol, value);
    return NIL;
}

void env_add_builtins(env *env, parse_data *parse_data) {
    env_add_builtin(parse_data, env, "+", builtin_add);
    env_add_builtin(parse_data, env, "-", builtin_sub);
    env_add_builtin(parse_data, env, "*", builtin_mul);
    env_add_builtin(parse_data, env, "/", builtin_div);

    env_add_builtin(parse_data, env, "define", builtin_define);
}

void free_symbol(symbol *sym) {
    while (sym) {
        symbol *t = sym->hash_next;
        my_free(sym->name);
        my_free(sym);
        sym = t;
    }
}

void free_lisp(parse_data *data) {
    symbol **symtab = data->symtab;
    symbol *sym;
    for (int i = 0; i < NHASH; ++i) {
        free_symbol(symtab[i]);
    }
    my_free(data->symtab);
}
