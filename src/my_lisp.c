#include "my_lisp.h"
#include <my_lisp.tab.h>

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

char *my_strdup(char *s) { return strdup(s); }

static object True = {.type = T_BOOLEAN, .bool_val = true, .ref_count = 1};
static object False = {.type = T_BOOLEAN, .bool_val = false, .ref_count = 1};
object *NIL = NULL;

object *new_error(const char *fmt, ...);
char *to_string(object *o, ...);
void free_object(object *o);
const char *object_type_name(object_type type);
object *assert_fun_arg_type(char *func, object *o, int i, object_type type);

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

#define ASSERT(cond, fmt, ...) !(cond) ? new_error(fmt, ##__VA_ARGS__) : NIL

object *assert_fun_arg_type(char *func, object *o, int i, object_type type) {
    if (!o || !(o->type & type)) {
        char *value = to_string(o);
        object *err = new_error("Function %s passed incorrect type for "
                                "argument %d. Got %s, Expected %s.",
                                func, i, value, object_type_name(type));
        my_free(value);
        return err;
    }
    unref(o);
    return NIL;
}

object *ref(object *o) {
    if (o) {
        o->ref_count++;
    }
    return o;
}

object *unref(object *o) {
    if (o && !(--o->ref_count)) {
        free_object(o);
        return NIL;
    }
    return o;
}

static inline object *new_object(object_type type) {
    object *o = my_malloc(sizeof(object));
    o->type = type;
    o->ref_count = 1;
    return o;
}

object *new_boolean(bool val) { return val ? ref(&True) : ref(&False); }

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
    object *ret_val = NIL;

    ERROR(assert_fun_arg_type("car", ref(list), 0, T_PAIR)) {
        ret_val = error;
        goto ret;
    }

    ret_val = ref(list->pair->car);
ret:
    unref(list);
    return ret_val;
}

object *setcar(object *list, object *car) {
    object *ret_val = NIL;

    ERROR(assert_fun_arg_type("setcar", ref(list), 0, T_PAIR)) {
        ret_val = error;
        unref(car);
        goto ret;
    }

    unref(list->pair->car);
    list->pair->car = car;

ret:
    unref(list);
    return ret_val;
}

object *cdr(object *list) {
    object *ret_val = NIL;

    ERROR(assert_fun_arg_type("cdr", ref(list), 0, T_PAIR)) {
        ret_val = error;
        goto ret;
    }

    ret_val = ref(list->pair->cdr);
ret:
    unref(list);
    return ret_val;
}

object *setcdr(object *list, object *cdr) {
    object *ret_val = NIL;

    ERROR(assert_fun_arg_type("setcdr", ref(list), 0, T_PAIR)) {
        ret_val = error;
        unref(cdr);
        goto ret;
    }

    unref(list->pair->cdr);
    list->pair->cdr = cdr;

ret:
    unref(list);
    return ret_val;
}

void free_pair(object *o) {
    unref(o->pair->car);
    unref(o->pair->cdr);
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

object *new_error(const char *fmt, ...) {
    object *o = new_object(T_ERR);
    error *err = my_malloc(sizeof(error));
    va_list args;
    va_start(args, fmt);
#define ERR_MSG_BUF_SIZE 512
    char buf[ERR_MSG_BUF_SIZE] = {'\0'};
    vsnprintf(buf, ERR_MSG_BUF_SIZE, fmt, args);
    va_end(args);

    err->msg = my_strdup(buf);
    o->err = err;
    return o;
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
    int i = 0;
    symbol *varg = NULL;

    object *param_list = NIL;
    if (params && params->type != T_PAIR) {
        varg = params->symbol;
    } else {
        object *ptr = NIL;
        object *arg = NIL;
        for_each_list(arg, params) {
            ERROR(ASSERT(arg->type == T_SYMBOL,
                         "compound proc: must be pass symbol as params")) {
                unref(param_list);
                unref(ptr);
                unref(params);
                unref(body);
                free_env(env);
                unref(idx);
                unref(arg);
                return error;
            }

            object *next = cdr(ref(idx));
            if (next && next->type != T_PAIR) {
                varg = next->symbol;
                unref(next);
                unref(idx);
                unref(arg);
                break;
            }
            unref(next);

            if (!param_list) {
                param_list = cons(ref(arg), NIL);
                ptr = ref(param_list);
            } else {
                setcdr(ref(ptr), cons(ref(arg), NIL));
                ptr = cdr(ptr);
            }

            i++;
        }
        unref(ptr);        
    }
    unref(params);

    object *o = new_object(T_COMPOUND_PROC);
    compound_proc *proc = my_malloc(sizeof(compound_proc));
    proc->param_count = i;
    proc->varg = varg;
    proc->parameters = param_list;
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
            return ref(e->objects[i]);
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
            unref(o);
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
            e->objects[i] = obj;
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
    e->objects[e->count] = obj;
    e->count++;
}

void free_object(object *o) {
    if (!o) {
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

char *list_to_string(object *list) {
    int len = 3; /* default "()" total 3 with memory */
    char *list_str = my_malloc(len);

    strcat(list_str, "(");
    object *o;
    for_each_list(o, list) {
        char *s = to_string(ref(o));
        int inc_len = 0;
        object *next = idx->type == T_PAIR ? cdr(ref(idx)) : NULL;
        char *cat_s = "";
        if (next) {
            if (next->type == T_PAIR) {
                inc_len = 1;
                cat_s = " ";
            } else {
                inc_len = 3;
                cat_s = " . ";
            }
            unref(next);
        }

        len += strlen(s) + inc_len;
        list_str = my_realloc(list_str, len);
        strcat(list_str, s);
        strcat(list_str, cat_s);
        my_free(s);
    }
    strcat(list_str, ")");
    unref(list);
    return list_str;
}

char *to_string(object *o, ...) {
    va_list args;
    va_start(args, o);
    env *e = va_arg(args, env *);
    va_end(args);

    if (!o) {
        unref(o);
        return my_strdup("()");
    }
#define BUF_SIZE 4096
    char buf[BUF_SIZE] = {'\0'};

    int len = 1;
    char *fmt = "%s";
    char *o_str = NULL;

    switch (o->type) {
    case T_PAIR:
        o_str = list_to_string(ref(o));
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
        symbol *symbol = env_get_sym(e, ref(o));
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

    /* len = snprintf(buf, BUF_SIZE, "%s(ref=%d)", str, o->ref_count); */
    /* str = my_realloc(str, len); */
    /* memcpy(str, buf, len); */
    unref(o);
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
    unref(list);
    return i;
}

object *proc_call(env *e, object *func, object *args) {
    object *ret_val = NIL;
    assert(func && func->type & (T_PRIMITIVE_PROC | T_COMPOUND_PROC));
    if (func->type == T_PRIMITIVE_PROC) {
        ret_val = func->primitive_proc->proc(e, ref(args));
    } else {
        env *func_env = func->compound_proc->env;
        object *params = ref(func->compound_proc->parameters);
        int total = func->compound_proc->param_count;

        symbol *varg_sym = func->compound_proc->varg;
        object *varg_val = NIL;
        object *ptr = NIL;
        object *given = NIL;
        int given_num = 0;
        object *param = NIL;
        for_each_list(given, args) {
            ERROR(ASSERT(params || varg_sym,
                         "Function passed too many arguments. "
                         "Expected %d.",
                         total)) {
                unref(idx);
                unref(given);
                unref(params);
                ret_val = error;
                goto ret;
            }

            // varg handle
            if (!params) {
                if (!varg_val) {
                    varg_val = cons(ref(given), NIL);
                    ptr = ref(varg_val);
                } else {
                    setcdr(ref(ptr), cons(ref(given), NIL));
                    ptr = cdr(ptr);
                }
                continue;
            }

            unref(param);
            param = car(ref(params));

            object *eval_val = eval(ref(given), e);
            ERROR(ref(eval_val)) {
                unref(idx);
                unref(given);
                unref(param);
                unref(params);
                unref(eval_val);
                ret_val = error;
                goto ret;
            }
            env_put(func_env, param->symbol, eval_val);
            given_num++;
            params = cdr(params);
        }

        unref(param);
        unref(params);
        unref(ptr);

        env_put(func_env, varg_sym, varg_val);

        ERROR(ASSERT(total == given_num,
                     "Exception: incorrect number of arguments")) {
            ret_val = error;
            goto ret;
        }

        ret_val = eval(ref(func->compound_proc->body), func_env);
    }

ret:
    unref(func);
    unref(args);
    return ret_val;
}

object *eval_list(object *expr, env *env) {
    object *operator= eval(car(ref(expr)), env);

    if (operator&& !(operator->type &(T_PRIMITIVE_PROC | T_COMPOUND_PROC))) {
        char *s = to_string(operator);
        object *err =
            new_error("Exception: attempt to apply non-procedure %s", s);
        my_free(s);
        unref(expr);
        return err;
    }

    object *operands = cdr(expr);
    return proc_call(env, operator, operands);
}

object *eval(object *exp, env *env) {
    object *ret_val = NIL;

    if (!exp) {
        ret_val = NIL;
        /* unref(exp); */
    } else if (exp->type == T_SYMBOL) {
        ret_val = env_get(env, exp->symbol);
        unref(exp);
    } else if (exp->type == T_PAIR) {
        ret_val = eval_list(exp, env);
    } else {
        ret_val = exp;
    }
    return ret_val;
}

void env_add_primitive(parse_data *data, env *env, char *name,
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
    case T_NUMBER:
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

object *primitive_op(env *e, object *args, char op) {
    object *ret_val = NIL;

    object *first = eval(car(ref(args)), e);
    ERROR(ref(first)) {
        unref(first);
        ret_val = error;
        goto ret;
    }

    int value = first->int_val;
    unref(first);

    int i = 1;
    object *operand;
    object *rest_args = cdr(ref(args));
    for_each_list(operand, rest_args) {
        object *o = eval(ref(operand), e);
        ERROR(ref(o)) {
            unref(idx);
            unref(operand);
            unref(o);
            ret_val = error;
            goto loop_exit;
        }

        char op_s[] = {op, '\0'};
        ERROR(assert_fun_arg_type(op_s, ref(o), i, T_NUMBER)) {
            unref(idx);
            unref(operand);
            unref(o);
            ret_val = error;
            goto loop_exit;
        }

        int eval_val = o->int_val;
        unref(o);
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
    ret_val = new_fix_number(value);

loop_exit:
    unref(rest_args);

ret:
    unref(args);
    return ret_val;
}

object *primitive_add(env *e, object *a) { return primitive_op(e, a, '+'); }

object *primitive_sub(env *e, object *a) { return primitive_op(e, a, '-'); }

object *primitive_mul(env *e, object *a) { return primitive_op(e, a, '*'); }

object *primitive_div(env *e, object *a) { return primitive_op(e, a, '/'); }

object *primitive_define(env *e, object *args) {
    object *ret_val = NIL;

    object *variable = car(ref(args));
    object *value = NIL;
    if (variable->type == T_SYMBOL) {
        value = cdr(ref(args));
        if (value != NIL) {
            unref(value);
            value = eval(car(cdr(ref(args))), e);
        } else {
            unref(value);
            value = NIL;
        }
    } else {
        unref(variable);
        variable = car(car(ref(args)));
        ERROR(ASSERT(variable->type == T_SYMBOL, "invalid syntax")) {
            ret_val = error;
            goto ret;
        }

        object *params = cdr(car(ref(args)));
        object *body = car(cdr(ref(args)));
        env *env = new_env();
        env->parent = e;
        value = new_compound_proc(env, params, body);
        ERROR(ref(value)) {
            unref(value);
            ret_val = error;
            goto ret;
        }
    }

    env_put(e, variable->symbol, value);

ret:
    unref(args);
    unref(variable);

    return ret_val;
}

object *is_type(object *o, object_type type) {
    object *ret_val = NIL;

    if (!o) {
        if (type == T_NULL) {
            ret_val = ref(&True);
        } else {
            ret_val = ref(&False);
        }
    } else if (o->type == T_ERR) {
        ret_val = ref(o);
    } else if (o->type & type) {
        ret_val = ref(&True);
    } else {
        ret_val = ref(&False);
    }

    unref(o);
    return ret_val;
}

object *assert_fun_args_count(char *fun, object *args, int count) {
    if (list_len(args) != count) {
        return new_error("Exception: incorrect argument count in call %s", fun);
    } else {
        return NIL;
    }
}

object *primitive_is_type(env *e, object *args, char *func, object_type type) {
    ERROR(assert_fun_args_count(func, ref(args), 1)) {
        unref(args);
        return error;
    }
    return is_type(eval(car(args), e), type);
}

object *primitive_is_boolean(env *e, object *args) {
    return primitive_is_type(e, args, "boolean?", T_BOOLEAN);
}

object *primitive_is_pair(env *e, object *args) {
    return primitive_is_type(e, args, "pair?", T_PAIR);
}

object *primitive_is_null(env *e, object *args) {
    return primitive_is_type(e, args, "null?", T_NULL);
}

object *primitive_is_number(env *e, object *args) {
    return primitive_is_type(e, args, "number?", T_NUMBER);
}

object *primitive_is_string(env *e, object *args) {
    return primitive_is_type(e, args, "string?", T_STRING);
}

object *primitive_is_procedure(env *e, object *args) {
    return primitive_is_type(e, args, "procedure?", T_PROCEDURE);
}

object *primitive_is_symbol(env *e, object *args) {
    return primitive_is_type(e, args, "symbol?", T_SYMBOL);
}

object *primitive_quote(env *e, object *args) {
    if (!args) {
        /* unref(args); */
        return NIL;
    }
    ERROR(assert_fun_args_count("quote", ref(args), 1)) {
        unref(args);
        return error;
    }
    return car(args);
}

object *primitive_cond(env *e, object *args) {
    return args;
}

void env_add_primitives(env *env, parse_data *parse_data) {
    env_add_primitive(parse_data, env, "boolean?", primitive_is_boolean);
    env_add_primitive(parse_data, env, "number?", primitive_is_number);
    env_add_primitive(parse_data, env, "string?", primitive_is_string);
    env_add_primitive(parse_data, env, "procedure?", primitive_is_procedure);
    env_add_primitive(parse_data, env, "pair?", primitive_is_pair);
    env_add_primitive(parse_data, env, "null?", primitive_is_null);
    env_add_primitive(parse_data, env, "symbol?", primitive_is_symbol);
    
    // todo
    env_add_primitive(parse_data, env, "char?", primitive_is_boolean);
    env_add_primitive(parse_data, env, "vector?", primitive_is_boolean);
    
    env_add_primitive(parse_data, env, "+", primitive_add);
    env_add_primitive(parse_data, env, "-", primitive_sub);
    env_add_primitive(parse_data, env, "*", primitive_mul);
    env_add_primitive(parse_data, env, "/", primitive_div);

    env_add_primitive(parse_data, env, "define", primitive_define);
    env_add_primitive(parse_data, env, "quote", primitive_quote);
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
    my_free(symtab);
}
