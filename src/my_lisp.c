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
    return ret;
}

static object True = {.type = T_BOOLEAN, .bool_val = true};
static object False = {.type = T_BOOLEAN, .bool_val = false};
object *NIL = NULL;

object *new_error(char *fmt, ...);
char *to_string(object *o);
const char *object_type_name(object_type type);
const char *type_name(object *o);
void del_object(object *o);
object *assert_fun_arg_type(char *func, object *o, int i, object_type type);

object *ref(object *o) {
    o->ref_count++;
    return o;
}
object *unref(object *o) {
    if (o && !(--o->ref_count)) {
        del_object(o);
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
    ref(symbol);
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

object *car(object *pair) {
    object *err = assert_fun_arg_type("car", pair, 0, T_PAIR);
    if (err) {
        return err;
    }
    return pair->pair->car;
}

object *cdr(object *pair) {
    object *err = assert_fun_arg_type("car", pair, 0, T_PAIR);
    if (err) {
        return err;
    }
    return pair->pair->cdr;
}

object *setcdr(object *list, object *cdr) {
    object *err = assert_fun_arg_type("car", list, 0, T_PAIR);
    if (err) {
        return err;
    }
    list->pair->cdr = cdr;
    return NIL;
}

void free_pair(pair *pair) {
    del_object(pair->car);
    del_object(pair->cdr);
    free(pair);
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

void free_string(string *s) {
    free(s->str_p);
    free(s);
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

void free_error(error *e) {
    free(e->msg);
    free(e);
}

object *new_primitive_proc(primitive_proc_ptr *proc, procedure procudure) {
    object *o = new_object(T_PRIMITIVE_PROC);
    primitive_proc *primitive = my_malloc(sizeof(primitive_proc));
    o->primitive_proc = primitive;
    primitive->procedure = procudure;
    primitive->proc = proc;
    return o;
}

void free_primitive_proc(primitive_proc *proc) { free(proc); }

object *new_compound_proc(env *env, object *params, object *body,
                          procedure procedure) {
    object *o = new_object(T_COMPOUND_PROC);
    compound_proc *proc = my_malloc(sizeof(compound_proc));
    proc->parameters = params;
    proc->body = body;
    proc->env = env;
    proc->procedure = procedure;
    o->compound_proc = proc;
    return o;
}

env *new_env(void) {
    env *e = my_malloc(sizeof(env));
    bzero(e, sizeof(env));
    return e;
}

void del_env(env *e) {
    for (int i = 0; i < e->count; i++) {
        /* free(e->symbols[i]); */
        unref(e->objects[i]);
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
            unref(e->objects[i]);
            e->objects[i] = ref(obj);
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
    e->objects[e->count] = ref(obj);
    e->count++;
}

void del_object(object *o) {
    if (o == NIL || o->ref_count) {
        return;
    }

    switch (o->type) {
    case T_ERR:
        free_error(o->err);
        break;
    case T_STRING:
        free_string(o->str);
        break;
    case T_PAIR:
        free_pair(o->pair);
        break;
    default:
        break;
    }

    free(o);
}

char *to_string(object *o);

char *pair_to_string(object *pair) {
    int len = 2;
    char *pair_str = malloc(len);

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
        pair_str = realloc(pair_str, len);
        strcat(pair_str, s);
        strcat(pair_str, cat_s);
        free(s);
    }
    strcat(pair_str, ")");
    return pair_str;
}

char *to_string(object *o) {
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
        o_str = o->primitive_proc->procedure.name;
        len += strlen(o_str) + strlen(fmt) - 2;
        break;
    default:
        break;
    }

    char *str = my_malloc(len);
    if (o_str) {
        snprintf(str, len, fmt, o_str);
    }

    if (o->type == T_PAIR && o_str) {
        free(o_str);
    }
    return str;
}

void object_print(object *o) {
    char *s = to_string(o);
    printf("%s", s);
    free(s);
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

object *eval_pair(object *expr, env *env) {
    object *o;
    object *eval_expr = NIL;
    object *pair_ptr = eval_expr;
    for_each_list(o, expr) {
        object *value = eval(o, env);
        if (value->type == T_ERR) {
            if (eval_expr != NIL) {
                del_object(eval_expr);
            }
            return value;
        }

        object *pair = cons(value, NIL);
        if (!eval_expr) {
            eval_expr = pair_ptr = pair;
        } else {
            // TODO: set cdr
            pair_ptr->pair->cdr = pair;
            pair_ptr = pair;
        }
    }

    object *operator= car(eval_expr);
    object *operands = cdr(eval_expr);

    if (operator&& !(operator->type &(T_PRIMITIVE_PROC | T_COMPOUND_PROC))) {
        del_object(eval_expr);
        char *s = to_string(operator);
        object *e =
            new_error("Exception: attempt to apply non-procedure %s", s);
        free(s);
        return e;
    }

    object *ret = proc_call(env, operator, operands);
    del_object(eval_expr);
    return ret;
}

object *eval(object *exp, env *env) {
    if (!exp) {
        return NULL;
    }

    if (exp->type == T_SYMBOL) {
        return env_get(env, exp->symbol);
    }  else if (exp->type == T_PAIR) {
        return eval_pair(exp, env);
    }
    return exp;
}

void env_add_builtin(parse_data *data, env *env, char *name,
                     primitive_proc_ptr *proc) {

    procedure procedure;
    procedure.name = name;
    env_put(env, lookup(data, name), new_primitive_proc(proc, procedure));
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

object *assert_fun_arg_type(char *func, object *o, int i, object_type type) {
    if (o && !(o->type & type)) {
        char *value = to_string(o);
        object *err = new_error("Function '%s' passed incorrect type for "
                                "argument %i. Got %s, Expected %s.",
                                func, to_string(o), i, object_type_name(type));
        free(value);
        return err;
    }
    return NULL;
}

object *builtin_op(env *e, object *args, char op) {
    int i = 0;

    int64_t value = car(args)->int_val;
    object *operand;
    for_each_list(operand, cdr(args)) {
        object *err = assert_fun_arg_type(&op, operand, i, T_FIXNUM | T_FLONUM);
        if (err) {
            return err;
        }

        switch (op) {
        case '+':
            value += operand->int_val;
            break;
        case '-':
            value -= operand->int_val;
            break;
        case '*':
            value *= operand->int_val;
            break;
        case '/':
            if (operand->int_val == 0) {
                return new_error("Division By Zero.");
            }
            value /= operand->int_val;
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
        if (variable->type == T_SYMBOL) {
            return new_error("invalid syntax");
        }
        object *params = car(cdr(args));
        object *body = cdr(args);
        procedure procedure = {.name = variable->symbol->name};

        env *env = new_env();
        env->parent = e;
        value = new_compound_proc(env, params, body, procedure);
    }

    env_put(e, variable->symbol, value);
    return NIL;
}

void env_add_builtins(env *env, parse_data *parse_data) {
    env_add_builtin(parse_data, env, "+", builtin_add);
    env_add_builtin(parse_data, env, "-", builtin_sub);
    env_add_builtin(parse_data, env, "*", builtin_mul);
    env_add_builtin(parse_data, env, "/", builtin_div);
}
