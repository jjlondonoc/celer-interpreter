#include "../include/eval.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // <-- necesario para malloc/free/calloc

// --- unescape de string literal de Celer ("...") ---
static char *unescape_celer_string(const char *lex) {
    // asume que lex viene como: "contenido con \n \t \\ \" "
    if(!lex) return NULL;
    size_t n = strlen(lex);
    size_t start = 0, end = n;
    if(n >= 2 && lex[0] == '\"' && lex[n-1] == '\"') { start = 1; end = n-1; }

    // tamaño máximo igual al contenido (escapes reducen o igualan)
    char *out = (char*)malloc((end - start) + 1);
    if(!out) return NULL;

    size_t oi = 0;
    for(size_t i = start; i < end; ++i) {
        char c = lex[i];
        if(c == '\\' && i + 1 < end) {
            char e = lex[i+1];
            switch(e) {
                case 'n':  out[oi++] = '\n'; break;
                case 't':  out[oi++] = '\t'; break;
                case '\\': out[oi++] = '\\'; break;
                case '\"': out[oi++] = '\"'; break;
                default:   out[oi++] = e;    break; // pasa tal cual el escape desconocido
            }
            i++; // saltar el char escapado
        } else {
            out[oi++] = c;
        }
    }
    out[oi] = '\0';
    return out;
}

static eval_result ok(value_t v){ eval_result r; r.sig=SIG_NONE; r.value=v; return r; }
static eval_result rt_err(void){ eval_result r; r.sig=SIG_RUNTIME_ERROR; r.value=v_void(); return r; }
static eval_result sig(eval_signal s){ eval_result r; r.sig=s; r.value=v_void(); return r; }
static eval_result sig_ret(value_t v){ eval_result r; r.sig=SIG_RETURN; r.value=v; return r; }

static value_t eval_expr(env_t *env, expr *e, eval_result *status);
static eval_result eval_stmt (env_t *env, stmt *s);
static eval_result eval_block(env_t *env, stmt *block);

static value_t call_function(env_t *env, const char *name, int argc, value_t *argv, eval_result *status);

// ----- builtin print -----
static value_t builtin_print(int argc, value_t *argv){
    for(int i=0;i<argc;i++){
        char *s=value_to_cstr(&argv[i]);
        if(s){ fputs(s, stdout); free(s); }
        if(i+1<argc) fputc(' ', stdout);
    }
    fputc('\n', stdout);
    return v_void();
}

// ----- helpers binarios -----
static value_t eval_binary_op(const value_t *L, op_kind op, const value_t *R){
    switch(op){
        case OP_ADD: return value_add(L,R);
        case OP_SUB: return value_sub(L,R);
        case OP_MUL: return value_mul(L,R);
        case OP_DIV: return value_div(L,R);
        case OP_MOD: return value_mod(L,R);
        case OP_EQ:  return value_eq (L,R);
        case OP_NEQ: return value_neq(L,R);
        case OP_LT:  return value_lt (L,R);
        case OP_LTE: return value_lte(L,R);
        case OP_GT:  return value_gt (L,R);
        case OP_GTE: return value_gte(L,R);
        case OP_AND: return value_and(L,R);
        case OP_OR:  return value_or (L,R);
        default: return v_void();
    }
}

// ----- expresiones -----
static value_t eval_expr(env_t *env, expr *e, eval_result *status){
    (void)status;
    switch(e->kind){
        case EXPR_IDENT: {
            value_t v;
            if(!env_get_var(env, e->as.ident.name, &v)) return v_void();
            return v;
        }
        case EXPR_INT_LIT:   return v_int(e->as.int_lit.value);
        case EXPR_FLOAT_LIT: return v_float(e->as.float_lit.value);
        case EXPR_BOOL_LIT:  return v_bool(e->as.bool_lit.value);
        case EXPR_STRING_LIT: {
            char *s = unescape_celer_string(e->as.string_lit.text);
            value_t v = v_string(s ? s : "");
            free(s);
            return v;
        }

        case EXPR_GROUPING:
            return eval_expr(env, e->as.grouping.inner, status);

        case EXPR_UNARY: {
            value_t R = eval_expr(env, e->as.unary.right, status);
            value_t out = v_void();
            if(e->as.unary.op == OP_NOT){
                out = value_not(&R);
            } else if(e->as.unary.op == OP_SUB){
                // -(R)  ===  0 - R
                value_t zero = v_int(0);
                out = value_sub(&zero, &R);
            }
            value_free(&R);
            return out;
        }

        case EXPR_BINARY: {
            value_t L = eval_expr(env, e->as.binary.left, status);
            value_t R = eval_expr(env, e->as.binary.right, status);
            value_t O = eval_binary_op(&L, e->as.binary.op, &R);
            value_free(&L); value_free(&R);
            return O;
        }

        case EXPR_ASSIGN: {
            value_t V = eval_expr(env, e->as.assign.value, status);
            if(e->as.assign.op == OP_ASSIGN){
                if(!env_set_var(env, e->as.assign.name, V)){
                    // si no existe, define mutable por defecto
                    env_define_var(env, e->as.assign.name, false, V);
                }
                return V;
            } else {
                value_t cur;
                if(!env_get_var(env, e->as.assign.name, &cur)) { value_free(&V); return v_void(); }
                value_t tmp = v_void();
                switch(e->as.assign.op){
                    case OP_PLUS_ASSIGN:    tmp = value_add(&cur, &V); break;
                    case OP_MINUS_ASSIGN:   tmp = value_sub(&cur, &V); break;
                    case OP_STAR_ASSIGN:    tmp = value_mul(&cur, &V); break;
                    case OP_SLASH_ASSIGN:   tmp = value_div(&cur, &V); break;
                    case OP_PERCENT_ASSIGN: tmp = value_mod(&cur, &V); break;
                    default: break;
                }
                value_free(&cur);
                env_set_var(env, e->as.assign.name, tmp);
                value_free(&V);
                return tmp;
            }
        }

        case EXPR_TERNARY: {
            value_t C = eval_expr(env, e->as.ternary.cond, status);
            value_t Cb = value_to_bool(&C);
            bool takeTrue = Cb.as.b;
            value_free(&C); value_free(&Cb);
            value_t out = takeTrue ? eval_expr(env, e->as.ternary.when_true, status)
                                   : eval_expr(env, e->as.ternary.when_false, status);
            return out;
        }

        case EXPR_CALL: {
            if(e->as.call.callee->kind != EXPR_IDENT) return v_void();
            const char *fname = e->as.call.callee->as.ident.name;
            int argc = (int)e->as.call.args.count;
            value_t *argv = (value_t*)calloc((size_t)argc, sizeof(value_t));
            for(int i=0;i<argc;i++) argv[i] = eval_expr(env, e->as.call.args.items[i], status);
            value_t ret = call_function(env, fname, argc, argv, status);
            for(int i=0;i<argc;i++) value_free(&argv[i]);
            free(argv);
            return ret;
        }
    }
    return v_void();
}

// ----- sentencias -----
static eval_result eval_stmt(env_t *env, stmt *s){
    switch(s->kind){
        case STMT_EXPR: {
            value_t v = eval_expr(env, s->as.expr_stmt.value, NULL);
            value_free(&v);
            return ok(v_void());
        }
        case STMT_RETURN: {
            value_t v = s->as.ret.value ? eval_expr(env, s->as.ret.value, NULL) : v_void();
            return sig_ret(v);
        }
        case STMT_BREAK: return sig(SIG_BREAK);
        case STMT_CONTINUE: return sig(SIG_CONTINUE);
        case STMT_BLOCK: return eval_block(env, s);
        case STMT_IF: {
            value_t c = eval_expr(env, s->as.if_stmt.cond, NULL);
            value_t cb = value_to_bool(&c); bool take = cb.as.b; value_free(&c); value_free(&cb);
            if(take) return eval_stmt(env, s->as.if_stmt.then_branch);
            if(s->as.if_stmt.else_branch) return eval_stmt(env, s->as.if_stmt.else_branch);
            return ok(v_void());
        }
        case STMT_FOR_WHILELIKE: {
            for(;;){
                value_t c = eval_expr(env, s->as.for_while.cond, NULL);
                value_t cb = value_to_bool(&c); bool cont = cb.as.b; value_free(&c); value_free(&cb);
                if(!cont) break;
                eval_result r = eval_stmt(env, s->as.for_while.body);
                if(r.sig==SIG_BREAK) { value_free(&r.value); break; }
                if(r.sig==SIG_RETURN || r.sig==SIG_RUNTIME_ERROR) return r;
                // SIG_CONTINUE cae aquí
            }
            return ok(v_void());
        }
        case STMT_FOR_CLIKE: {
            if(s->as.for_clike.init){
                eval_result r = eval_stmt(env, s->as.for_clike.init);
                if(r.sig!=SIG_NONE) return r;
            }
            for(;;){
                if(s->as.for_clike.cond){
                    value_t c = eval_expr(env, s->as.for_clike.cond, NULL);
                    value_t cb = value_to_bool(&c); bool cont = cb.as.b; value_free(&c); value_free(&cb);
                    if(!cont) break;
                }
                eval_result rbody = eval_stmt(env, s->as.for_clike.body);
                if(rbody.sig==SIG_BREAK) { value_free(&rbody.value); break; }
                if(rbody.sig==SIG_RETURN || rbody.sig==SIG_RUNTIME_ERROR) return rbody;
                if(s->as.for_clike.post){
                    value_t v = eval_expr(env, s->as.for_clike.post, NULL);
                    value_free(&v);
                }
            }
            return ok(v_void());
        }
    }
    return rt_err();
}

static eval_result eval_block(env_t *env, stmt *block){
    env_t *local = env_new(env); // nuevo scope
    for(size_t i=0;i<block->as.block.stmts.count;i++){
        eval_result r = eval_stmt(local, block->as.block.stmts.items[i]);
        if(r.sig!=SIG_NONE){ env_free(local); return r; }
    }
    env_free(local);
    return ok(v_void());
}

// ----- funciones -----
static value_t call_user_function(env_t *env, func_decl *fn, int argc, value_t *argv, eval_result *status){
    (void)status; // no lo usamos por ahora
    env_t *local = env_new(env);

    size_t pc = fn->params.count;
    for(size_t i=0;i<pc;i++){
        const char *pname = fn->params.items[i].name;
        value_t arg = (i<(size_t)argc) ? argv[i] : v_void();
        env_define_var(local, pname, false, arg);
    }

    eval_result r = eval_stmt(local, fn->body);
    env_free(local);

    if(r.sig==SIG_RETURN){
        return r.value; // ownership al caller
    }
    value_free(&r.value);
    return v_void();
}

static value_t call_function(env_t *env, const char *name, int argc, value_t *argv, eval_result *status){
    (void)status;
    // builtin?
    builtin_fn b = env_get_builtin(env, name);
    if(b) return b(argc, argv);
    // user?
    func_decl *fn = env_get_func(env, name);
    if(!fn) return v_void();
    return call_user_function(env, fn, argc, argv, status);
}

// ----- programa -----

eval_result eval_program(env_t *global, const program_ast *P){
    // define builtins (idempotente simple)
    env_define_builtin(global, "print", builtin_print);

    // Cargar vars y funcs globales (top-level)
    func_decl *main_local = NULL; // <-- main de ESTE chunk
    for(size_t i=0;i<P->decls.count;i++){
        decl *d = P->decls.items[i];
        if(d->kind==DECL_VAR){
            value_t v = d->as.var.init ? eval_expr(global, d->as.var.init, NULL) : v_void();
            env_define_var(global, d->as.var.name, d->as.var.is_const, v);
            value_free(&v);
        } else if(d->kind==DECL_FUNC){
            env_define_func(global, d->as.func.name, &d->as.func);
            if(strcmp(d->as.func.name, "main") == 0) {
                main_local = &d->as.func; // guarda la main de este programa
            }
        }
    }

    // Ejecutar la main del chunk si existe
    if(main_local){
        value_t r = call_user_function(global, main_local, 0, NULL, NULL);
        value_free(&r);
    }

    return ok(v_void());
}

/*eval_result eval_program(env_t *global, const program_ast *P){
    // define builtins
    env_define_builtin(global, "print", builtin_print);

    // Cargar vars y funcs globales (top-level)
    for(size_t i=0;i<P->decls.count;i++){
        decl *d = P->decls.items[i];
        if(d->kind==DECL_VAR){
            value_t v = d->as.var.init ? eval_expr(global, d->as.var.init, NULL) : v_void();
            env_define_var(global, d->as.var.name, d->as.var.is_const, v);
            value_free(&v);
        } else if(d->kind==DECL_FUNC){
            env_define_func(global, d->as.func.name, &d->as.func);
        }
    }

    // Ejecutar main() si existe
    func_decl *main = env_get_func(global, "main");
    if(main){
        value_t r = call_user_function(global, main, 0, NULL, NULL);
        value_free(&r);
    }

    return ok(v_void());
}*/

