#include "my_lisp.h"
#include <my_lisp.tab.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <my-os/list.h>

    static inline object *
    object_list_entry(object *list) {
    assert(list);
    return list->type == T_PAIR ? car(list) : list;
}

static inline bool object_list_has_next(object *list) {
    bool ret_val = list;
    unref(list);
    return ret_val;
}

static inline object *object_list_next(object *idx) {
    assert(idx);
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
bool object_symbol_equal(object *sym, char *s);

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

number *make_number(enum exactness exactness, u64 real, u64 imaginary) {
    number *number = my_malloc(sizeof(struct number_t));
    number->exactness = exactness;
    number->real_part = real;
    number->imaginary_part = imaginary;
    return number;
}

object *new_number(number *number) {
    object *o = new_object(T_NUMBER);
    o->number = number;
    return o;
}

void free_number(object *o) { my_free(o->number); }

enum exactness to_exactness_flag(char c) {
    enum exactness exactness_flag = EXACTNESS_UNKOWN;
    switch (c) {
    case 'e':
    case 'E':
        exactness_flag = EXACTNESS_FIX;
        break;
    case 'i':
    case 'I':
        exactness_flag = EXACTNESS_FLO;
        break;
    }
    return exactness_flag;
}

enum radix to_radix_flag(char c) {
    enum radix radix_flag = RADIX_10;
    switch (c) {
    case 'b':
    case 'B':
        radix_flag = RADIX_2;
        break;
    case 'o':
    case 'O':
        radix_flag = RADIX_8;
        break;
    case 'd':
    case 'D':
        radix_flag = RADIX_10;
        break;
    case 'x':
    case 'X':
        radix_flag = RADIX_16;
        break;
    }
    return radix_flag;
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
        for_each_object_list_entry(arg, params) {
            ERROR(ASSERT(arg && arg->type == T_SYMBOL,
                         "compound proc: must be pass symbol as params")) {
                unref(param_list);
                unref(ptr);
                unref(body);
                free_env(env);
                unref(params);
                unref(idx);
                unref(arg);
                return error;
            }

            if (!param_list) {
                param_list = cons(ref(arg), NIL);
                ptr = ref(param_list);
            } else {
                setcdr(ref(ptr), cons(ref(arg), NIL));
                ptr = cdr(ptr);
            }
            i++;

            object *next = cdr(ref(idx));
            if (next && next->type != T_PAIR) {
                varg = next->symbol;
                unref(next);
                unref(idx);
                unref(arg);
                break;
            }
            unref(next);
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

object *new_macro_proc(object *literals, object *syntax_rules) {
    object *o = new_object(T_MACRO_PROC);
    macro_proc *proc = my_malloc(sizeof(macro_proc));
    proc->literals = literals;
    proc->syntax_rules = syntax_rules;
    o->macro_proc = proc;
    return o;
}

void free_macro_proc(object *o) {
    macro_proc *proc = o->macro_proc;
    unref(proc->literals);
    unref(proc->syntax_rules);
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
        unref(o);
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
    case T_NUMBER:
        free_number(o);
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
        break;
    case T_MACRO_PROC:
        free_macro_proc(o);
        break;
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
    for_each_object_list_entry(o, list) {
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

char *number_to_string(object *o) {
#define NUMBER_BUF_SIZE 1024
    char buf[NUMBER_BUF_SIZE] = {'\0'};
    number *number = o->number;
    assert(number->exactness != EXACTNESS_UNKOWN);

    switch (number->exactness) {
    case EXACTNESS_FIX:
        if (number->imaginary_part == 0) {
            snprintf(buf, NUMBER_BUF_SIZE, "%Li", (long long)number->real_part);
        } else {
            snprintf(buf, NUMBER_BUF_SIZE, "%Li%+Lii", (long long)number->real_part,
                     (long long)number->imaginary_part);
        }
        break;
    case EXACTNESS_FLO:
        if (number->imaginary_part == 0) {
            snprintf(buf, NUMBER_BUF_SIZE, "%f", (double)number->real_part);
        } else {
            snprintf(buf, NUMBER_BUF_SIZE, "%f%+fi", (double)number->real_part,
                     (double)number->imaginary_part);
        }
        break;
    }
    unref(o);
    return my_strdup(buf);
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
    case T_NUMBER: {
        o_str = number_to_string(ref(o));
        len += strlen(o_str);
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
    case T_COMPOUND_PROC: {
        symbol *symbol = env_get_sym(e, ref(o));
        if (symbol) {
            fmt = "#<procedure %s>";
            o_str = symbol->name;
            len += strlen(o_str) + strlen(fmt) - 2;
        } else {
            fmt = "#<procedure>";
            o_str = "";
            len += strlen(fmt);
        }
        break;
    }
    default:
        break;
    }

    char *str = my_malloc(len);
    if (o_str) {
        snprintf(str, len, fmt, o_str);
        if (o->type == T_PAIR || o->type == T_NUMBER) {
            my_free(o_str);
        }
    }

    unref(o);
    return str;
}

void object_print(object *o, env *e) {
    char *s = to_string(o, e);
    printf("%s", s);
    my_free(s);
}

size_t object_list_len(object *list) {
    if (!list)
        return 0;

    size_t i = 0;
    object *o;
    for_each_object_list_entry(o, list) { i++; }
    unref(list);
    return i;
}

object *compound_proc_call(env *e, object *func, object *args,
                           parse_data *data) {
    object *ret_val = NIL;

    env *func_env = func->compound_proc->env;
    int total = func->compound_proc->param_count;
    symbol *varg_sym = func->compound_proc->varg;
    int given_num = 0;

    object *params = ref(func->compound_proc->parameters);
    object *param = NIL;

    object *varg_val = NIL;
    object *ptr = NIL;

    object *given = NIL;
    for_each_object_list_entry(given, args) {
        unref(param);
        param = params ? car(ref(params)) : NIL;

        ERROR(ASSERT(param || varg_sym,
                     "Function passed too many arguments. "
                     "Expected %d.",
                     total)) {
            unref(idx);
            unref(given);
            unref(param);
            unref(params);
            ret_val = error;
            goto ret;
        }

        // varg handle
        if (!param) {
            if (!varg_val) {
                varg_val = cons(ref(given), NIL);
                ptr = ref(varg_val);
            } else {
                setcdr(ref(ptr), cons(ref(given), NIL));
                ptr = cdr(ptr);
            }
            continue;
        }

        object *eval_val = eval(ref(given), e, data);
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

    if (varg_sym) {
        env_put(func_env, varg_sym, varg_val);
    }

    ERROR(
        ASSERT(total == given_num,
               "Exception: incorrect number of arguments, given: %d, total: %d",
               given_num, total)) {
        ret_val = error;
        goto ret;
    }

    ret_val = eval(ref(func->compound_proc->body), func_env, data);

ret:
    unref(func);
    unref(args);
    return ret_val;
}

typedef struct pattern_bind_t {
    symbol *sym;
    object *value;
    struct list_head head;
} pattern_bind;

void free_pattern_bind(pattern_bind *pattern_binds) {
    while (!list_empty(&pattern_binds->head)) {
        pattern_bind *entry = list_next_entry(pattern_binds, head);
        list_del(&entry->head);
        unref(entry->value);
        my_free(entry);
    }
    my_free(pattern_binds);
}

pattern_bind *pattern_bind_get(pattern_bind *binds, symbol *sym) {
    pattern_bind *entry;
    list_for_each_entry(entry, &binds->head, head) {
        if (entry->sym == sym) {
            return entry;
        }
    }
    return NULL;
}

typedef enum pattern_match_code {
    MATCH,
    NOT_MATCH,
    SYNTAX_ERR
} pattern_match_code;

object *object_list_append(object *list, object *o) {
    assert(!list || list->type == T_PAIR);

    object *ret_val = NIL;
    object *last_idx = NIL;
    for_each_object_list(list) {
        unref(last_idx);
        last_idx = ref(idx);
    }

    if (last_idx == NIL) {
        ret_val = cons(o, NIL);
    } else {
        assert(cdr(ref(last_idx)) == NIL);
        setcdr(last_idx, cons(o, NIL));
        ret_val = ref(list);
    }
    unref(list);
    return ret_val;
}

void collect_pattern(object *pattern, pattern_bind *pattern_values) {
    if (!pattern) {
        return;
    }

    if (pattern->type == T_SYMBOL) {
        pattern_bind *entry = pattern_bind_get(pattern_values, pattern->symbol);
        if (!entry) {
            entry = my_malloc(sizeof(pattern_bind));
            entry->sym = pattern->symbol;
            entry->value = NIL;
            list_add(&entry->head, &pattern_values->head);
        }
    } else if (pattern->type == T_PAIR) {
        collect_pattern(car(ref(pattern)), pattern_values);
        collect_pattern(cdr(ref(pattern)), pattern_values);
    }
    unref(pattern);
    // TODO: vector
}

pattern_match_code match_ident_pattern(pattern_bind *pattern_binds,
                                       object *literals, object *pattern,
                                       object *expr, parse_data *data) {
    pattern_match_code ret_val = MATCH;
    symbol *underscore = lookup(data, "_");

    // TODO: to object list contain function

    object *literal;
    for_each_object_list_entry(literal, literals) {
        if (literal && literal->symbol == pattern->symbol) {
            if (!(expr && expr->type == T_SYMBOL &&
                  literal->symbol == expr->symbol)) {
                ret_val = NOT_MATCH;
            }
            unref(idx);
            unref(literal);
            goto ret;
        }
    }

    if (pattern->symbol != underscore) {
        pattern_bind *entry = pattern_bind_get(pattern_binds, pattern->symbol);
        if (!entry) {
            entry = my_malloc(sizeof(pattern_bind));
            entry->sym = pattern->symbol;
            entry->value = NIL;
            list_add(&entry->head, &pattern_binds->head);
        }

        entry->value = object_list_append(entry->value, ref(expr));
    }

ret:
    unref(literals);
    unref(pattern);
    unref(expr);
    return ret_val;
}
pattern_match_code match_pattern(pattern_bind *pattern_binds, object *literals,
                                 object *pattern, object *expr,
                                 parse_data *data);

bool object_symbol_eq(object *obj_sym, symbol *sym) {
    bool ret_val = false;
    if (obj_sym && obj_sym->type == T_SYMBOL && obj_sym->symbol == sym) {
        ret_val = true;
    }
    unref(obj_sym);
    return ret_val;
}

pattern_match_code match_list_pattern(pattern_bind *pattern_binds,
                                      object *literals, object *pattern_list,
                                      object *expr, parse_data *data) {

    pattern_match_code ret_val = MATCH;

    symbol *ellipsis = lookup(data, "...");

    object *pattern;
    object *expr_idx = expr;
    for_each_object_list_entry(pattern, pattern_list) {
        object *next_idx = object_list_next(ref(idx));
        if (object_list_has_next(next_idx) &&
            object_symbol_eq(object_list_entry(object_list_next(ref(idx))),
                             ellipsis)) {

            int num =
                object_list_len(ref(expr_idx)) - object_list_len(ref(idx)) + 2;
            if (num == 0) {
                collect_pattern(ref(pattern), pattern_binds);
            }

            while (num-- > 0) {
                if (match_pattern(pattern_binds, ref(literals), ref(pattern),
                                  object_list_entry(ref(expr_idx)),
                                  data) == NOT_MATCH) {
                    unref(idx);
                    unref(pattern);
                    unref(expr_idx);

                    ret_val = NOT_MATCH;
                    goto ret;
                }
                expr_idx = object_list_next(expr_idx);
            }
            idx = object_list_next(idx);
        } else {
            if (object_list_has_next(ref(expr_idx)) &&
                match_pattern(pattern_binds, ref(literals), ref(pattern),
                              object_list_entry(ref(expr_idx)),
                              data) == MATCH) {
                expr_idx = object_list_next(expr_idx);
            } else {
                unref(idx);
                unref(pattern);
                unref(expr_idx);

                ret_val = NOT_MATCH;
                goto ret;
            }
        }
    }

    if (object_list_has_next(expr_idx)) {
        ret_val = NOT_MATCH;
    }

ret:
    unref(literals);
    unref(pattern_list);

    return ret_val;
}

pattern_match_code match_pattern(pattern_bind *pattern_binds, object *literals,
                                 object *pattern, object *expr,
                                 parse_data *data) {
    pattern_match_code code = NOT_MATCH;

    if (pattern) {
        switch (pattern->type) {
        case T_PAIR:
            code = match_list_pattern(pattern_binds, ref(literals),
                                      ref(pattern), ref(expr), data);
            break;
        case T_SYMBOL:
            code = match_ident_pattern(pattern_binds, ref(literals),
                                       ref(pattern), ref(expr), data);
            break;
        case T_STRING:
        case T_NUMBER:
            // TODO: impl equal?
            code = NOT_MATCH;
            break;
        default:
            code = SYNTAX_ERR;
            break;
        }
    } else {
        if (!expr) {
            code = MATCH;
        }
    }
    unref(literals);
    unref(pattern);
    unref(expr);
    return code;
}

pattern_match_code match_srpattern(pattern_bind **pattern_binds,
                                   object *literals, object *srpattern,
                                   object *expr, parse_data *data) {

    if (!*pattern_binds) {
        *pattern_binds = my_malloc(sizeof(pattern_bind));
        INIT_LIST_HEAD(&(*pattern_binds)->head);
    }

    // ignore first pattern
    pattern_match_code code =
        match_pattern(*pattern_binds, literals, cdr(srpattern), expr, data);
    if (code != MATCH) {
        free_pattern_bind(*pattern_binds);
        *pattern_binds = NULL;
    }
    return code;
}

object *object_list_entry_ref(object *list, size_t index) {
    object *ret_val = NIL;
    int i = -1;
    object *o;
    for_each_object_list_entry(o, list) {
        i++;
        if (i == index) {
            ret_val = o;
            unref(idx);
            break;
        }
    }

    if (i != index) {
        ret_val = new_error("list len = %d < index = %d", i, index);
    }

    unref(list);
    return ret_val;
}

typedef enum transform_tempalte_code {
    TTC_SYNTAX_ERR,
    TTC_INDEX_RANGE_ERR,
    TTC_OK
} transform_tempalte_code;

transform_tempalte_code transform_template(object **result,
                                           pattern_bind *pattern_binds,
                                           object *template, size_t index,
                                           bool is_ellipsis_ident,
                                           parse_data *data);

transform_tempalte_code
transform_list_template(object **result, pattern_bind *pattern_binds,
                        object *template_list, size_t index,
                        bool is_ellipsis_ident, parse_data *data) {
    assert(*result == NIL);

    transform_tempalte_code ret_val = TTC_OK;

    symbol *ellipsis = lookup(data, "...");
    object *template;

    object *ptr = NIL;
    for_each_object_list_entry(template, template_list) {
        object *next_idx = object_list_next(ref(idx));
        if (object_list_has_next(next_idx) &&
            object_symbol_eq(object_list_entry(object_list_next(ref(idx))),
                             ellipsis) &&
            !is_ellipsis_ident) {
            int i = 0;
            object *ret = NIL;
            while ((ret_val = transform_template(
                        &ret, pattern_binds, ref(template), i,
                        is_ellipsis_ident, data)) == TTC_OK) {
                if (!*result) {
                    *result = cons(ret, NIL);
                    ptr = ref(*result);
                } else {
                    setcdr(ref(ptr), cons(ret, NIL));
                    ptr = cdr(ptr);
                }
                ret = NIL;
                i++;
            }

            if (ret_val == TTC_SYNTAX_ERR) {
                unref(idx);
                unref(template);
                break;
            } else if (ret_val == TTC_INDEX_RANGE_ERR) {
                ret_val = TTC_OK;
            }

            idx = object_list_next(idx);
        } else {
            object *ret = NIL;
            ret_val = transform_template(&ret, pattern_binds, ref(template),
                                         index, is_ellipsis_ident, data);
            if (ret_val == TTC_OK) {
                if (!*result) {
                    *result = cons(ret, NIL);
                    ptr = ref(*result);
                } else {
                    setcdr(ref(ptr), cons(ret, NIL));
                    ptr = cdr(ptr);
                }
            } else {
                assert(ret == NIL);

                unref(idx);
                unref(template);
                break;
            }
        }
    }

    if (ret_val != TTC_OK) {
        unref(*result);
        *result = NIL;
    }

    unref(ptr);
    unref(template_list);
    return ret_val;
}

transform_tempalte_code
transform_symbol_template(object **result, pattern_bind *pattern_binds,
                          object *template, size_t index, parse_data *data) {
    assert(*result == NIL);
    assert(template && template->type == T_SYMBOL);

    transform_tempalte_code code = TTC_OK;
    pattern_bind *entry = pattern_bind_get(pattern_binds, template->symbol);

    if (entry) {
        object *ret = object_list_entry_ref(ref(entry->value), index);
        ERROR(ref(ret)) {
            unref(error);
            unref(ret);
            code = TTC_INDEX_RANGE_ERR;
            goto ret;
        }
        *result = ret;
    } else {
        *result = ref(template);
    }

ret:
    unref(template);
    return code;
}

// is_ellipsis_ident
transform_tempalte_code transform_template(object **result,
                                           pattern_bind *pattern_binds,
                                           object *template, size_t index,
                                           bool is_ellipsis_ident,
                                           parse_data *data) {
    assert(*result == NIL);

    transform_tempalte_code ret_val = TTC_OK;

    symbol *ellipsis = lookup(data, "...");

    if (template) {
        switch (template->type) {
        case T_PAIR:
            if (object_symbol_eq(object_list_entry(ref(template)), ellipsis) &&
                !is_ellipsis_ident) {
                object *next_idx = object_list_next(ref(template));
                if (object_list_has_next(ref(next_idx))) {
                    ret_val = transform_template(result, pattern_binds,
                                                 object_list_entry(next_idx),
                                                 index, true, data);
                } else {
                    unref(next_idx);
                    ret_val = TTC_SYNTAX_ERR;
                }
            } else {
                ret_val = transform_list_template(result, pattern_binds,
                                                  ref(template), index,
                                                  is_ellipsis_ident, data);
            }

            break;
        case T_SYMBOL:
            ret_val = transform_symbol_template(result, pattern_binds,
                                                ref(template), index, data);
            break;
        case T_STRING:
        case T_NUMBER:
            *result = ref(template);
            break;
        default:
            break;
        }
    } else {
        *result = NIL;
    }
    unref(template);
    return ret_val;
}

object *macro_proc_call(env *e, object *func, object *args, parse_data *data) {
    macro_proc *proc = func->macro_proc;

    /* printf("literals: "); */
    /* object_print(ref(proc->literals), e); */
    /* printf("\n"); */
    /* printf("syntax rules: "); */
    /* object_print(ref(proc->syntax_rules), e); */
    /* printf("\n"); */

    /* printf("args: "); */
    /* object_print(ref(args), e); */
    /* printf("\n"); */

    object *ret_val = NIL;
    transform_tempalte_code ttcode = TTC_SYNTAX_ERR;
    object *syntax_rule;
    for_each_object_list_entry(syntax_rule, proc->syntax_rules) {
        object *srpattern = car(ref(syntax_rule));

        /* printf("srpattern: "); */
        /* object_print(cdr(ref(srpattern)), e); */
        /* printf("\n"); */

        pattern_bind *pattern_binds = NULL;
        pattern_match_code ret = match_srpattern(
            &pattern_binds, ref(proc->literals), srpattern, ref(args), data);

        if (ret == MATCH) {
            pattern_bind *entry;
            /* list_for_each_entry(entry, &pattern_binds->head, head) { */
            /*     char *s = to_string(ref(entry->value)); */
            /*     printf("%s = %s\n", entry->sym->name, s); */
            /*     my_free(s); */
            /* } */
            object *template = car(cdr(ref(syntax_rule)));
            /* printf("template: "); */
            /* object_print(ref(template), e); */
            /* printf("\n"); */

            ttcode = transform_template(&ret_val, pattern_binds, template, 0,
                                        false, data);
            free_pattern_bind(pattern_binds);
            unref(syntax_rule);
            unref(idx);
            break;
        }
    }

    if (ttcode != TTC_OK) {
        ret_val = new_error("Exception: invalid syntax");
    } else {
        printf("template value: ");
        object_print(ref(ret_val), e);
        printf("\n");

        ret_val = eval(ret_val, e, data);
    }

    unref(args);
    unref(func);

    return ret_val;
}

object *proc_call(env *e, object *func, object *args, parse_data *data) {
    object *ret_val = NIL;
    assert(func && func->type & (T_PROCEDURE | T_MACRO_PROC));
    switch (func->type) {
    case T_PRIMITIVE_PROC:
        ret_val = func->primitive_proc->proc(e, ref(args), data);
        break;
    case T_COMPOUND_PROC:
        ret_val = compound_proc_call(e, ref(func), ref(args), data);
        break;
    case T_MACRO_PROC:
        ret_val = macro_proc_call(e, ref(func), ref(args), data);
        break;
    default:
        ret_val = new_error("Exception: func type is not supported");
        break;
    }

ret:
    unref(func);
    unref(args);
    return ret_val;
}

object *eval_list(object *expr, env *env, parse_data *data) {
    object *operator= eval(car(ref(expr)), env, data);

    if (operator&& !(operator->type &(T_PROCEDURE | T_MACRO_PROC))) {
        char *s = to_string(operator);
        object *err =
            new_error("Exception: attempt to apply non-procedure %s", s);
        my_free(s);
        unref(expr);
        return err;
    }

    object *operands = cdr(expr);
    return proc_call(env, operator, operands, data);
}

object *eval(object *exp, env *env, parse_data *data) {
    object *ret_val = NIL;

    if (!exp) {
        ret_val = NIL;
        /* unref(exp); */
    } else if (exp->type == T_SYMBOL) {
        ret_val = env_get(env, exp->symbol);
        unref(exp);
    } else if (exp->type == T_PAIR) {
        ret_val = eval_list(exp, env, data);
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

object *primitive_op(env *e, object *args, char op, parse_data *data) {
    object *ret_val = NIL;
    char op_s[] = {op, '\0'};

    object *first = eval(car(ref(args)), e, data);

    ERROR(assert_fun_arg_type(op_s, ref(first), 0, T_NUMBER)) {
        unref(first);
        ret_val = error;
        goto ret;
    }

    int value = first->number->real_part;
    unref(first);

    int i = 1;
    object *operand;
    object *rest_args = cdr(ref(args));
    for_each_object_list_entry(operand, rest_args) {
        object *o = eval(ref(operand), e, data);
        ERROR(ref(o)) {
            unref(idx);
            unref(operand);
            unref(o);
            ret_val = error;
            goto loop_exit;
        }

        ERROR(assert_fun_arg_type(op_s, ref(o), i, T_NUMBER)) {
            unref(idx);
            unref(operand);
            unref(o);
            ret_val = error;
            goto loop_exit;
        }

        int eval_val = o->number->real_part;
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
    ret_val = new_number(make_number(EXACTNESS_FIX, 0, 0));

loop_exit:
    unref(rest_args);

ret:
    unref(args);
    return ret_val;
}

object *primitive_add(env *e, object *a, parse_data *data) {
    return primitive_op(e, a, '+', data);
}

object *primitive_sub(env *e, object *a, parse_data *data) {
    return primitive_op(e, a, '-', data);
}

object *primitive_mul(env *e, object *a, parse_data *data) {
    return primitive_op(e, a, '*', data);
}

object *primitive_div(env *e, object *a, parse_data *data) {
    return primitive_op(e, a, '/', data);
}

object *primitive_define(env *e, object *args, parse_data *data) {
    object *ret_val = NIL;

    object *variable = car(ref(args));
    object *value = NIL;
    if (variable->type == T_SYMBOL) {
        value = cdr(ref(args));
        if (value != NIL) {
            unref(value);
            value = eval(car(cdr(ref(args))), e, data);
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
        object *begin = new_symbol(lookup(data, "begin"));
        object *body = cons(begin, cdr(ref(args)));

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
    if (object_list_len(args) != count) {
        return new_error("Exception: incorrect argument count in call %s", fun);
    } else {
        return NIL;
    }
}

object *assert_fun_args_count_range(char *fun, object *args, int min, int max) {
    int size = object_list_len(args);
    if (min <= size && size <= max) {
        return new_error("Exception: incorrect argument count in call %s", fun);
    } else {
        return NIL;
    }
}

object *primitive_is_type(env *e, object *args, char *func, object_type type,
                          parse_data *data) {
    ERROR(assert_fun_args_count(func, ref(args), 1)) {
        unref(args);
        return error;
    }
    return is_type(eval(car(args), e, data), type);
}

object *primitive_is_boolean(env *e, object *args, parse_data *data) {
    return primitive_is_type(e, args, "boolean?", T_BOOLEAN, data);
}

object *primitive_is_pair(env *e, object *args, parse_data *data) {
    return primitive_is_type(e, args, "pair?", T_PAIR, data);
}

object *primitive_is_null(env *e, object *args, parse_data *data) {
    return primitive_is_type(e, args, "null?", T_NULL, data);
}

object *primitive_is_number(env *e, object *args, parse_data *data) {
    return primitive_is_type(e, args, "number?", T_NUMBER, data);
}

object *primitive_is_string(env *e, object *args, parse_data *data) {
    return primitive_is_type(e, args, "string?", T_STRING, data);
}

object *primitive_is_procedure(env *e, object *args, parse_data *data) {
    return primitive_is_type(e, args, "procedure?", T_PROCEDURE, data);
}

object *primitive_is_symbol(env *e, object *args, parse_data *data) {
    return primitive_is_type(e, args, "symbol?", T_SYMBOL, data);
}

object *primitive_is_error(env *e, object *args, parse_data *data) {
    ERROR(assert_fun_args_count("error?", ref(args), 1)) {
        unref(args);
        return error;
    }

    object *ret_val = NIL;
    object *param = eval(car(args), e, data);
    if (param && param->type == T_ERR) {
        ret_val = ref(&True);
    } else {
        ret_val = ref(&False);
    }
    unref(param);
    return ret_val;
}

object *primitive_quote(env *e, object *args, parse_data *data) {
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

object *primitive_if(env *e, object *args, parse_data *data) {
    object *ret_val = NIL;

    size_t arg_len = object_list_len(ref(args));

    ERROR(ASSERT(2 <= arg_len && arg_len <= 3, "Exception: invalid syntax")) {
        ret_val = error;
        goto ret;
    }

    object *test = eval(car(ref(args)), e, data);
    object *consequent = car(cdr(ref(args)));
    object *alternate = arg_len == 3 ? car(cdr(cdr(ref(args)))) : NIL;

    ERROR(ref(test)) {
        unref(test);
        unref(consequent);
        unref(alternate);
        ret_val = error;
        goto ret;
    }
    if (test != &False) {
        // true
        unref(alternate);
        ret_val = eval(consequent, e, data);
    } else {
        // false
        unref(consequent);
        ret_val = alternate ? eval(alternate, e, data) : NIL;
    }
    unref(test);

ret:
    unref(args);
    return ret_val;
}

bool object_symbol_equal(object *sym, char *s) {
    if (!sym || sym->type != T_SYMBOL) {
        unref(sym);
        return false;
    }

    bool ret_val = false;
    if (!strcmp(sym->symbol->name, s)) {
        ret_val = true;
    } else {
        ret_val = false;
    }
    unref(sym);
    return ret_val;
}

object *make_if(object *test, object *consequent, object *alternate,
                parse_data *data) {
    assert(consequent);

    object *ret_val;
    object *sym_if = new_symbol(lookup(data, "if"));
    if (alternate) {
        ret_val =
            cons(sym_if, cons(test, cons(consequent, cons(alternate, NIL))));
    } else {
        ret_val = cons(sym_if, cons(test, cons(consequent, NIL)));
    }
    return ret_val;
}

object *primitive_cond(env *e, object *args, parse_data *data) {
    object *ret_val = NIL;

    object *clause = NIL;

    object *sym_begin = new_symbol(lookup(data, "begin"));

    object *test = NIL;
    object *consequent = NIL;
    object *ptr;

    object *cond_to_if = NIL;

    for_each_object_list_entry(clause, args) {
        test = car(ref(clause));
        if (object_symbol_equal(ref(test), "else")) {
            object *rest = cdr(ref(idx));
            ERROR(ASSERT(!rest, "else clause isn't last")) {
                unref(idx);
                unref(clause);

                unref(rest);
                unref(test);

                ret_val = error;
                goto ret;
            }
            /* unref(rest); */

            object *expression = cons(ref(sym_begin), cdr(ref(clause)));
            if (!cond_to_if) {
                cond_to_if = expression;
            } else {
                setcdr(ref(ptr), cons(expression, NIL));
            }
            unref(idx);
            unref(clause);
            unref(test);
            break;
        }

        /* test = eval(test, e, data); */
        object *arrow = car(cdr(ref(clause)));
        if (object_symbol_equal(arrow, "=>")) {
            // (<test> => <expression>)
            // expression = procedure object
            // TODO: impl
            ERROR(ASSERT(object_list_len(ref(clause)) == 3,
                         "Exception: misplaced aux keyword =>")) {
                unref(idx);
                unref(clause);

                ret_val = error;
                goto ret;
            }

            consequent = cons(car(cdr(cdr(ref(clause)))), test);
        } else {
            // (<test> => <expression>*)
            consequent = cons(ref(sym_begin), cdr(ref(clause)));
        }

        if (!cond_to_if) {
            cond_to_if = make_if(test, consequent, NIL, data);
            ptr = cdr(cdr(ref(cond_to_if)));
        } else {
            setcdr(ref(ptr), cons(make_if(test, consequent, NIL, data), NIL));
            ptr = cdr(cdr(car(cdr(ptr))));
        }
    }

    ret_val = eval(ref(cond_to_if), e, data);

ret:
    unref(cond_to_if);
    unref(ptr);
    unref(args);
    unref(sym_begin);

    return ret_val;
}

object *primitive_begin(env *e, object *args, parse_data *data) {
    object *ret_val = NIL;
    object *form = NIL;
    for_each_object_list_entry(form, args) {
        unref(ret_val);
        ret_val = eval(ref(form), e, data);
    }
    unref(args);
    return ret_val;
}

object *primitive_car(env *e, object *args, parse_data *data) {
    ERROR(assert_fun_args_count("car", ref(args), 1)) {
        unref(args);
        return error;
    }
    return car(eval(car(args), e, data));
}

object *primitive_cdr(env *e, object *args, parse_data *data) {
    ERROR(assert_fun_args_count("cdr", ref(args), 1)) {
        unref(args);
        return error;
    }
    return cdr(eval(car(args), e, data));
}

object *primitive_cons(env *e, object *args, parse_data *data) {
    ERROR(assert_fun_args_count("cons", ref(args), 2)) {
        unref(args);
        return error;
    }
    return cons(eval(car(ref(args)), e, data), eval(car(cdr(args)), e, data));
}

object *primitive_lambda(env *e, object *args, parse_data *data) {
    object *params = car(ref(args));
    object *begin = new_symbol(lookup(data, "begin"));
    object *body = cons(begin, cdr(args));
    env *env = new_env();
    env->parent = e;

    return new_compound_proc(env, params, body);
}

object *primitive_define_syntax(env *e, object *args, parse_data *data) {
    return primitive_define(e, args, data);
}

object *primitive_syntax_rules(env *e, object *args, parse_data *data) {

    object *ret_val = NIL;

    // TODO: assert args count

    symbol *ellipsis = lookup(data, "...");
    symbol *underscore = lookup(data, "...");

    object *literals = car(ref(args));
    object *literal;
    for_each_object_list_entry(literal, literals) {
        // error check
        ERROR(ASSERT(literal && literal->type == T_SYMBOL &&
                         literal->symbol != ellipsis &&
                         literal->symbol != underscore,
                     "")) {
            unref(idx);
            unref(literal);
            unref(literals);
            ret_val = error;
            goto ret;
        }
    }

    object *syntax_rules = cdr(args);
    ret_val = new_macro_proc(literals, syntax_rules);

    // TODO: 检查语法错误
    /*     object *syntax_rule; */
    /*     for_each_object_list_entry(syntax_rule, syntax_rules) { */
    /*         object *srpattern = car(syntax_rule); */
    /*         object *template = car(cdr(syntax_rule)); */
    /*     } */
    /* error: */
ret:
    return ret_val;
}

object *primitive_let(env *e, object *args, parse_data *data) {

    object *ret_val = NIL;

    object *bindings = car(ref(args));
    ERROR(ASSERT(!bindings || bindings->type == T_PAIR, "invalid syntax let")) {
        unref(bindings);
        ret_val = error;
        goto ret;
    }

    env *let_env = new_env();

    object *binding;
    for_each_object_list_entry(binding, bindings) {
        ERROR(
            ASSERT(object_list_len(ref(binding)) == 2, "invalid syntax let")) {
            unref(binding);
            unref(idx);
            ret_val = error;
            goto ret;
        }

        object *var = car(ref(binding));

        ERROR(ASSERT(var && var->type == T_SYMBOL, "invalid syntax let")) {
            unref(var);
            unref(binding);
            unref(idx);
            ret_val = error;
            goto ret;
        }

        object *value = env_get(let_env, var->symbol);
        ERROR(ASSERT(value && value->type == T_ERR, "invalid syntax let")) {
            unref(value);
            unref(var);
            unref(binding);
            unref(idx);
            ret_val = error;
            goto ret;
        }
        object *init = car(cdr(ref(binding)));
        object *val = eval(init, e, data);
        ERROR(ref(val)) {
            unref(value);
            unref(var);
            unref(binding);
            unref(idx);
            ret_val = error;
            goto ret;
        }
        env_put(let_env, var->symbol, val);

        unref(value);
        unref(var);
    }

    let_env->parent = e;

    object *body = cdr(ref(args));
    ret_val = primitive_begin(let_env, body, data);

ret:
    unref(bindings);
    unref(args);
    free_env(let_env);
    return ret_val;
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

    env_add_primitive(parse_data, env, "error?", primitive_is_error);

    env_add_primitive(parse_data, env, "+", primitive_add);
    env_add_primitive(parse_data, env, "-", primitive_sub);
    env_add_primitive(parse_data, env, "*", primitive_mul);
    env_add_primitive(parse_data, env, "/", primitive_div);

    env_add_primitive(parse_data, env, "define", primitive_define);
    env_add_primitive(parse_data, env, "quote", primitive_quote);

    env_add_primitive(parse_data, env, "begin", primitive_begin);

    env_add_primitive(parse_data, env, "car", primitive_car);
    env_add_primitive(parse_data, env, "cdr", primitive_cdr);
    env_add_primitive(parse_data, env, "cons", primitive_cons);

    env_add_primitive(parse_data, env, "lambda", primitive_lambda);

    env_add_primitive(parse_data, env, "if", primitive_if);
    env_add_primitive(parse_data, env, "cond", primitive_cond);

    env_add_primitive(parse_data, env, "let", primitive_let);

    env_add_primitive(parse_data, env, "define-syntax",
                      primitive_define_syntax);
    env_add_primitive(parse_data, env, "syntax-rules", primitive_syntax_rules);
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
