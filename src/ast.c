#include "../include/ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ---------------- helpers ----------------
static char *dup_cstr(const char *s){
    if(!s){ char *z = (char*)malloc(1); if(z) z[0]='\0'; return z; }
    size_t n = strlen(s);
    char *p = (char*)malloc(n+1);
    if(!p) return NULL;
    memcpy(p, s, n+1);
    return p;
}

static void *xrealloc(void *ptr, size_t newcap, size_t elem){
    return realloc(ptr, newcap * elem);
}

static void expr_vec_push(expr_vec *v, expr *e){
    if(v->count == v->cap){
        size_t nc = v->cap ? v->cap*2u : 4u;
        v->items = (expr**)xrealloc(v->items, nc, sizeof(expr*));
        v->cap = nc;
    }
    v->items[v->count++] = e;
}

static void stmt_vec_push(stmt_vec *v, stmt *s){
    if(v->count == v->cap){
        size_t nc = v->cap ? v->cap*2u : 4u;
        v->items = (stmt**)xrealloc(v->items, nc, sizeof(stmt*));
        v->cap = nc;
    }
    v->items[v->count++] = s;
}

static void param_vec_push(param_vec *v, const char *name, type_spec t){
    if(v->count == v->cap){
        size_t nc = v->cap ? v->cap*2u : 4u;
        v->items = (param_decl*)xrealloc(v->items, nc, sizeof(param_decl));
        v->cap = nc;
    }
    v->items[v->count].name = dup_cstr(name);
    v->items[v->count].type = t;
    v->count++;
}

// ---------------- expr ctor ----------------
expr *expr_ident(const char *name, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_IDENT; e->line=line; e->col=col;
    e->as.ident.name = dup_cstr(name);
    return e;
}
expr *expr_int(long long v, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_INT_LIT; e->line=line; e->col=col;
    e->as.int_lit.value = v;
    return e;
}
expr *expr_float(double v, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_FLOAT_LIT; e->line=line; e->col=col;
    e->as.float_lit.value = v;
    return e;
}
expr *expr_bool(bool v, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_BOOL_LIT; e->line=line; e->col=col;
    e->as.bool_lit.value = v;
    return e;
}
expr *expr_string(const char *text, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_STRING_LIT; e->line=line; e->col=col;
    e->as.string_lit.text = dup_cstr(text);
    return e;
}
expr *expr_unary(op_kind op, expr *right, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_UNARY; e->line=line; e->col=col;
    e->as.unary.op = op; e->as.unary.right = right;
    return e;
}
expr *expr_binary(expr *left, op_kind op, expr *right, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_BINARY; e->line=line; e->col=col;
    e->as.binary.left = left; e->as.binary.op = op; e->as.binary.right = right;
    return e;
}
expr *expr_assign(const char *name, op_kind op, expr *value, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_ASSIGN; e->line=line; e->col=col;
    e->as.assign.name = dup_cstr(name);
    e->as.assign.op = op;
    e->as.assign.value = value;
    return e;
}
expr *expr_group(expr *inner, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_GROUPING; e->line=line; e->col=col;
    e->as.grouping.inner = inner;
    return e;
}
expr *expr_ternary(expr *cond, expr *when_true, expr *when_false, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_TERNARY; e->line=line; e->col=col;
    e->as.ternary.cond = cond;
    e->as.ternary.when_true = when_true;
    e->as.ternary.when_false = when_false;
    return e;
}
expr *expr_call(expr *callee, int line, int col){
    expr *e = (expr*)calloc(1, sizeof(*e));
    e->kind = EXPR_CALL; e->line=line; e->col=col;
    e->as.call.callee = callee;
    e->as.call.args.items = NULL; e->as.call.args.count=0; e->as.call.args.cap=0;
    return e;
}
void expr_args_push(expr *call_expr, expr *arg){
    if(!call_expr || call_expr->kind != EXPR_CALL) return;
    expr_vec_push(&call_expr->as.call.args, arg);
}

// ---------------- stmt ctor ----------------
stmt *stmt_expr_stmt(expr *e, int line, int col){
    stmt *s = (stmt*)calloc(1, sizeof(*s));
    s->kind = STMT_EXPR; s->line=line; s->col=col;
    s->as.expr_stmt.value = e;
    return s;
}
stmt *stmt_return(expr *e, int line, int col){
    stmt *s = (stmt*)calloc(1, sizeof(*s));
    s->kind = STMT_RETURN; s->line=line; s->col=col;
    s->as.ret.value = e;
    return s;
}
stmt *stmt_break(int line, int col){
    stmt *s = (stmt*)calloc(1, sizeof(*s));
    s->kind = STMT_BREAK; s->line=line; s->col=col;
    return s;
}
stmt *stmt_continue(int line, int col){
    stmt *s = (stmt*)calloc(1, sizeof(*s));
    s->kind = STMT_CONTINUE; s->line=line; s->col=col;
    return s;
}
stmt *stmt_block(void){
    stmt *s = (stmt*)calloc(1, sizeof(*s));
    s->kind = STMT_BLOCK; s->line=0; s->col=0;
    s->as.block.stmts.items=NULL; s->as.block.stmts.count=0; s->as.block.stmts.cap=0;
    return s;
}
void stmt_block_push(stmt *block, stmt *st){
    if(!block || block->kind != STMT_BLOCK) return;
    stmt_vec_push(&block->as.block.stmts, st);
}
stmt *stmt_if(expr *cond, stmt *then_branch, stmt *else_branch, int line, int col){
    stmt *s = (stmt*)calloc(1, sizeof(*s));
    s->kind = STMT_IF; s->line=line; s->col=col;
    s->as.if_stmt.cond = cond;
    s->as.if_stmt.then_branch = then_branch;
    s->as.if_stmt.else_branch = else_branch;
    return s;
}
stmt *stmt_for_while(expr *cond, stmt *body, int line, int col){
    stmt *s = (stmt*)calloc(1, sizeof(*s));
    s->kind = STMT_FOR_WHILELIKE; s->line=line; s->col=col;
    s->as.for_while.cond = cond;
    s->as.for_while.body = body;
    return s;
}
stmt *stmt_for_clike(stmt *init, expr *cond, expr *post, stmt *body, int line, int col){
    stmt *s = (stmt*)calloc(1, sizeof(*s));
    s->kind = STMT_FOR_CLIKE; s->line=line; s->col=col;
    s->as.for_clike.init = init;
    s->as.for_clike.cond = cond;
    s->as.for_clike.post = post;
    s->as.for_clike.body = body;
    return s;
}

// ---------------- decl ctor ----------------
decl *decl_var(const char *name, bool is_const, type_spec t, expr *init, int line, int col){
    decl *d = (decl*)calloc(1, sizeof(*d));
    d->kind = DECL_VAR;
    d->as.var.name = dup_cstr(name);
    d->as.var.is_const = is_const;
    d->as.var.type = t;
    d->as.var.init = init;
    d->as.var.line = line; d->as.var.col = col;
    return d;
}
decl *decl_func(const char *name, type_spec ret_type, int line, int col){
    decl *d = (decl*)calloc(1, sizeof(*d));
    d->kind = DECL_FUNC;
    d->as.func.name = dup_cstr(name);
    d->as.func.ret_type = ret_type;
    d->as.func.params.items=NULL; d->as.func.params.count=0; d->as.func.params.cap=0;
    d->as.func.body = NULL;
    d->as.func.line = line; d->as.func.col = col;
    return d;
}
void decl_func_param_push(decl *fn, const char *name, type_spec t){
    if(!fn || fn->kind != DECL_FUNC) return;
    param_vec_push(&fn->as.func.params, name, t);
}
void decl_func_set_body(decl *fn, stmt *body){
    if(!fn || fn->kind != DECL_FUNC) return;
    fn->as.func.body = body;
}

// ---------------- program ----------------
program_ast program_make(void){
    program_ast p; p.decls.items=NULL; p.decls.count=0; p.decls.cap=0; return p;
}
void program_push_decl(program_ast *p, decl *d){
    if(p->decls.count == p->decls.cap){
        size_t nc = p->decls.cap ? p->decls.cap*2u : 4u;
        p->decls.items = (decl**)xrealloc(p->decls.items, nc, sizeof(decl*));
        p->decls.cap = nc;
    }
    p->decls.items[p->decls.count++] = d;
}

// ---------------- free ----------------
void expr_free(expr *e){
    if(!e) return;
    switch(e->kind){
        case EXPR_IDENT: free(e->as.ident.name); break;
        case EXPR_INT_LIT: case EXPR_FLOAT_LIT: case EXPR_BOOL_LIT: break;
        case EXPR_STRING_LIT: free(e->as.string_lit.text); break;
        case EXPR_UNARY: expr_free(e->as.unary.right); break;
        case EXPR_BINARY: expr_free(e->as.binary.left); expr_free(e->as.binary.right); break;
        case EXPR_ASSIGN: free(e->as.assign.name); expr_free(e->as.assign.value); break;
        case EXPR_GROUPING: expr_free(e->as.grouping.inner); break;
        case EXPR_TERNARY: expr_free(e->as.ternary.cond); expr_free(e->as.ternary.when_true); expr_free(e->as.ternary.when_false); break;
        case EXPR_CALL:
            expr_free(e->as.call.callee);
            for(size_t i=0;i<e->as.call.args.count;i++) expr_free(e->as.call.args.items[i]);
            free(e->as.call.args.items);
            break;
    }
    free(e);
}
void stmt_free(stmt *s){
    if(!s) return;
    switch(s->kind){
        case STMT_EXPR: expr_free(s->as.expr_stmt.value); break;
        case STMT_RETURN: expr_free(s->as.ret.value); break;
        case STMT_BREAK: case STMT_CONTINUE: break;
        case STMT_BLOCK:
            for(size_t i=0;i<s->as.block.stmts.count;i++) stmt_free(s->as.block.stmts.items[i]);
            free(s->as.block.stmts.items);
            break;
        case STMT_IF:
            expr_free(s->as.if_stmt.cond);
            stmt_free(s->as.if_stmt.then_branch);
            stmt_free(s->as.if_stmt.else_branch);
            break;
        case STMT_FOR_WHILELIKE:
            expr_free(s->as.for_while.cond);
            stmt_free(s->as.for_while.body);
            break;
        case STMT_FOR_CLIKE:
            stmt_free(s->as.for_clike.init);
            expr_free(s->as.for_clike.cond);
            expr_free(s->as.for_clike.post);
            stmt_free(s->as.for_clike.body);
            break;
    }
    free(s);
}
void decl_free(decl *d){
    if(!d) return;
    if(d->kind == DECL_VAR){
        free(d->as.var.name);
        expr_free(d->as.var.init);
    } else {
        free(d->as.func.name);
        for(size_t i=0;i<d->as.func.params.count;i++) free(d->as.func.params.items[i].name);
        free(d->as.func.params.items);
        stmt_free(d->as.func.body);
    }
    free(d);
}
void program_free(program_ast *p){
    if(!p) return;
    for(size_t i=0;i<p->decls.count;i++) decl_free(p->decls.items[i]);
    free(p->decls.items);
    p->decls.items=NULL; p->decls.count=p->decls.cap=0;
}

// ---------------- pretty print ----------------
static const char* tname(type_kind k){
    switch(k){
        case TYPE_VOID: return "void";
        case TYPE_INT: return "int";
        case TYPE_BOOL: return "bool";
        case TYPE_FLOAT: return "float";
        case TYPE_STRING: return "string";
        default: return "?";
    }
}
static const char* opname(op_kind op){
    switch(op){
        case OP_ASSIGN: return "=";
        case OP_PLUS_ASSIGN: return "+=";
        case OP_MINUS_ASSIGN: return "-=";
        case OP_STAR_ASSIGN: return "*=";
        case OP_SLASH_ASSIGN: return "/=";
        case OP_PERCENT_ASSIGN: return "%=";
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_EQ: return "==";
        case OP_NEQ: return "!=";
        case OP_LT: return "<";
        case OP_LTE: return "<=";
        case OP_GT: return ">";
        case OP_GTE: return ">=";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        case OP_NOT: return "!";
        default: return "?";
    }
}

static void indent(int n){ for(int i=0;i<n;i++) putchar(' '); }

static void print_expr(const expr *e, int ind);

static void print_call(const expr *e, int ind){
    indent(ind); printf("Call:\n");
    indent(ind+2); printf("callee:\n");
    print_expr(e->as.call.callee, ind+4);
    indent(ind+2); printf("args:\n");
    for(size_t i=0;i<e->as.call.args.count;i++){
        print_expr(e->as.call.args.items[i], ind+4);
    }
}

static void print_expr(const expr *e, int ind){
    if(!e){ indent(ind); printf("(null-expr)\n"); return; }
    switch(e->kind){
        case EXPR_IDENT: indent(ind); printf("Ident %s\n", e->as.ident.name); break;
        case EXPR_INT_LIT: indent(ind); printf("Int %lld\n", e->as.int_lit.value); break;
        case EXPR_FLOAT_LIT: indent(ind); printf("Float %g\n", e->as.float_lit.value); break;
        case EXPR_BOOL_LIT: indent(ind); printf("Bool %s\n", e->as.bool_lit.value?"true":"false"); break;
        case EXPR_STRING_LIT: indent(ind); printf("String %s\n", e->as.string_lit.text); break;
        case EXPR_GROUPING:
            indent(ind); printf("Group\n");
            print_expr(e->as.grouping.inner, ind+2);
            break;
        case EXPR_UNARY:
            indent(ind); printf("Unary %s\n", opname(e->as.unary.op));
            print_expr(e->as.unary.right, ind+2);
            break;
        case EXPR_BINARY:
            indent(ind); printf("Binary %s\n", opname(e->as.binary.op));
            print_expr(e->as.binary.left, ind+2);
            print_expr(e->as.binary.right, ind+2);
            break;
        case EXPR_ASSIGN:
            indent(ind); printf("Assign %s\n", opname(e->as.assign.op));
            indent(ind+2); printf("name: %s\n", e->as.assign.name);
            print_expr(e->as.assign.value, ind+2);
            break;
        case EXPR_TERNARY:
            indent(ind); printf("Ternary\n");
            indent(ind+2); printf("cond:\n");
            print_expr(e->as.ternary.cond, ind+4);
            indent(ind+2); printf("true:\n");
            print_expr(e->as.ternary.when_true, ind+4);
            indent(ind+2); printf("false:\n");
            print_expr(e->as.ternary.when_false, ind+4);
            break;
        case EXPR_CALL:
            print_call(e, ind);
            break;
    }
}

static void print_stmt(const stmt *s, int ind){
    if(!s){ indent(ind); printf("(null-stmt)\n"); return; }
    switch(s->kind){
        case STMT_EXPR:
            indent(ind); printf("ExprStmt\n");
            print_expr(s->as.expr_stmt.value, ind+2);
            break;
        case STMT_RETURN:
            indent(ind); printf("Return\n");
            print_expr(s->as.ret.value, ind+2);
            break;
        case STMT_BREAK: indent(ind); printf("Break\n"); break;
        case STMT_CONTINUE: indent(ind); printf("Continue\n"); break;
        case STMT_BLOCK:
            indent(ind); printf("Block\n");
            for(size_t i=0;i<s->as.block.stmts.count;i++)
                print_stmt(s->as.block.stmts.items[i], ind+2);
            break;
        case STMT_IF:
            indent(ind); printf("If\n");
            indent(ind+2); printf("cond:\n");
            print_expr(s->as.if_stmt.cond, ind+4);
            indent(ind+2); printf("then:\n");
            print_stmt(s->as.if_stmt.then_branch, ind+4);
            indent(ind+2); printf("else:\n");
            print_stmt(s->as.if_stmt.else_branch, ind+4);
            break;
        case STMT_FOR_WHILELIKE:
            indent(ind); printf("For(while-like)\n");
            indent(ind+2); printf("cond:\n");
            print_expr(s->as.for_while.cond, ind+4);
            indent(ind+2); printf("body:\n");
            print_stmt(s->as.for_while.body, ind+4);
            break;
        case STMT_FOR_CLIKE:
            indent(ind); printf("For(C-like)\n");
            indent(ind+2); printf("init:\n");
            print_stmt(s->as.for_clike.init, ind+4);
            indent(ind+2); printf("cond:\n");
            print_expr(s->as.for_clike.cond, ind+4);
            indent(ind+2); printf("post:\n");
            print_expr(s->as.for_clike.post, ind+4);
            indent(ind+2); printf("body:\n");
            print_stmt(s->as.for_clike.body, ind+4);
            break;
    }
}

void ast_print_program(const program_ast *p){
    printf("Program\n");
    for(size_t i=0;i<p->decls.count;i++){
        const decl *d = p->decls.items[i];
        if(d->kind == DECL_VAR){
            printf(" VarDecl %s %s\n", d->as.var.is_const ? "const":"variable", d->as.var.name);
            printf("  type: %s\n", tname(d->as.var.type.kind));
            printf("  init:\n");
            print_expr(d->as.var.init, 4);
        } else {
            printf(" FuncDecl %s -> %s\n", d->as.func.name, tname(d->as.func.ret_type.kind));
            printf("  params:\n");
            for(size_t j=0;j<d->as.func.params.count;j++){
                printf("    - %s : %s\n", d->as.func.params.items[j].name, tname(d->as.func.params.items[j].type.kind));
            }
            printf("  body:\n");
            print_stmt(d->as.func.body, 4);
        }
    }
}
