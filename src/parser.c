#include "../include/parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ---------- utils ----------
static char *dup_cstr(const char *s){
    if(!s){ char *z=(char*)malloc(1); if(z) z[0]='\0'; return z; }
    size_t n=strlen(s); char *p=(char*)malloc(n+1); if(!p) return NULL; memcpy(p,s,n+1); return p;
}
static void perr_push(parse_error_list *lst, int line, int col, const char *msg){
    if(lst->count==lst->cap){ size_t nc=lst->cap?lst->cap*2u:4u; lst->items=(parse_error*)realloc(lst->items, nc*sizeof(parse_error)); lst->cap=nc; }
    lst->items[lst->count].line=line;
    lst->items[lst->count].col=col;
    lst->items[lst->count].message=dup_cstr(msg?msg:"");
    lst->count++;
}

// ---------- token helpers ----------
static void advance_tok(parser_t *ps){
    if(ps->prev.lexeme) token_free(&ps->prev);
    ps->prev = ps->curr;
    ps->curr = lexer_next_token(ps->lx);
}
static bool check(parser_t *ps, token_type t){ return ps->curr.type==t; }
static bool match(parser_t *ps, token_type t){ if(check(ps,t)){ advance_tok(ps); return true; } return false; }

static void error_at_current(parser_t *ps, const char *msg){
    ps->had_error = true;
    perr_push(&ps->errors, ps->curr.line, ps->curr.column, msg);
}
static void error_at_previous(parser_t *ps, const char *msg){
    ps->had_error = true;
    perr_push(&ps->errors, ps->prev.line, ps->prev.column, msg);
}

static void consume_or_err(parser_t *ps, token_type t, const char *expect_msg){
    if(match(ps,t)) return;
    error_at_current(ps, expect_msg);
    // modo pánico simple: intenta avanzar si token illegal para no quedar pegado
    if(ps->curr.type==TOK_ILLEGAL) advance_tok(ps);
}

static type_spec parse_type_spec(parser_t *ps); // fwd
static stmt* parse_block_stmt(parser_t *ps);    // fwd
static stmt* parse_statement(parser_t *ps);     // fwd
static decl* parse_declaration(parser_t *ps);   // fwd
static expr* parse_expression(parser_t *ps);    // fwd

// ---------- precedence table ----------
typedef enum {
    PREC_LOWEST = 0,
    PREC_ASSIGN,    // =
    PREC_OR,        // ||
    PREC_AND,       // &&
    PREC_EQUALITY,  // == !=
    PREC_COMPARE,   // < <= > >=
    PREC_TERM,      // + -
    PREC_FACTOR,    // * / %
    PREC_UNARY,     // ! -
    PREC_CALL,      // ()
    PREC_PRIMARY
} precedence;

// ---------- parser init/dispose ----------
void parser_init(parser_t *ps, lexer_t *lx){
    memset(ps,0,sizeof(*ps));
    ps->lx = lx;
    ps->curr = token_from_cstr(TOK_ILLEGAL,"<uninit>",0,0);
    ps->prev = token_from_cstr(TOK_ILLEGAL,"<uninit>",0,0);
    advance_tok(ps); // carga curr
}
void parser_dispose(parser_t *ps){
    if(ps->curr.lexeme) token_free(&ps->curr);
    if(ps->prev.lexeme) token_free(&ps->prev);
    for(size_t i=0;i<ps->errors.count;i++) free(ps->errors.items[i].message);
    free(ps->errors.items);
    memset(ps,0,sizeof(*ps));
}
const parse_error_list* parser_errors(const parser_t *ps){ return &ps->errors; }

// ---------- helpers de sincronización ----------
static void synchronize(parser_t *ps){
    // avanza hasta ; o } o inicio razonable
    while(ps->curr.type!=TOK_EOF){
        if(ps->prev.type==TOK_SEMICOLON) return;
        switch(ps->curr.type){
            case TOK_VARIABLE: case TOK_CONST: case TOK_Function:
            case TOK_IF: case TOK_FOR: case TOK_RETURN:
            case TOK_BREAK: case TOK_CONTINUE:
            case TOK_RBRACE:
                return;
            default: break;
        }
        advance_tok(ps);
    }
}

// ---------- mapeos ----------
static op_kind op_from_token(token_type t){
    switch(t){
        case TOK_ASSIGN: return OP_ASSIGN;
        case TOK_PLUS_ASSIGN: return OP_PLUS_ASSIGN;
        case TOK_MINUS_ASSIGN: return OP_MINUS_ASSIGN;
        case TOK_STAR_ASSIGN: return OP_STAR_ASSIGN;
        case TOK_SLASH_ASSIGN: return OP_SLASH_ASSIGN;
        case TOK_PERCENT_ASSIGN: return OP_PERCENT_ASSIGN;

        case TOK_PLUS: return OP_ADD;
        case TOK_MINUS:return OP_SUB;
        case TOK_STAR: return OP_MUL;
        case TOK_SLASH:return OP_DIV;
        case TOK_PERCENT:return OP_MOD;

        case TOK_EQ:   return OP_EQ;
        case TOK_NEQ:  return OP_NEQ;
        case TOK_LT:   return OP_LT;
        case TOK_LTE:  return OP_LTE;
        case TOK_GT:   return OP_GT;
        case TOK_GTE:  return OP_GTE;

        case TOK_AND:  return OP_AND;
        case TOK_OR:   return OP_OR;
        case TOK_NOT:  return OP_NOT;
        default: return OP_NONE;
    }
}

static type_spec parse_type_spec(parser_t *ps){
    if(match(ps, TOK_INT))    return type_make(TYPE_INT);
    if(match(ps, TOK_BOOL))   return type_make(TYPE_BOOL);
    if(match(ps, TOK_FLOAT))  return type_make(TYPE_FLOAT);
    if(match(ps, TOK_STRING)) return type_make(TYPE_STRING);
    // permitir void sólo en retorno de función; no distinguimos aquí
    // lo haremos permisivo para que el parser de función lo use.
    // (si quieres prohibirlo en variables, valida en semántica).
    if(ps->curr.type==TOK_IDENT && strcmp(ps->curr.lexeme,"void")==0){
        advance_tok(ps);
        return type_make(TYPE_VOID);
    }
    error_at_current(ps, "Tipo esperado (int, bool, float, string)");
    return type_make(TYPE_VOID);
}

// ---------- expresiones (Pratt) ----------
static expr* parse_precedence(parser_t *ps, precedence prec);

static expr* parse_primary(parser_t *ps){
    if(match(ps, TOK_IDENT)){
        // podría ser inicio de llamada (se resuelve posteriormente en parse_call_suffix)
        return expr_ident(ps->prev.lexeme, ps->prev.line, ps->prev.column);
    }
    if(match(ps, TOK_INT_LIT)){
        long long v = atoll(ps->prev.lexeme);
        return expr_int(v, ps->prev.line, ps->prev.column);
    }
    if(match(ps, TOK_FLOAT_LIT)){
        double d = atof(ps->prev.lexeme);
        return expr_float(d, ps->prev.line, ps->prev.column);
    }
    if(match(ps, TOK_STRING_LIT)){
        return expr_string(ps->prev.lexeme, ps->prev.line, ps->prev.column);
    }
    if(match(ps, TOK_TRUE))  return expr_bool(true,  ps->prev.line, ps->prev.column);
    if(match(ps, TOK_FALSE)) return expr_bool(false, ps->prev.line, ps->prev.column);

    if(match(ps, TOK_LPAREN)){
        int l = ps->prev.line, c = ps->prev.column;
        expr *inner = parse_expression(ps);
        consume_or_err(ps, TOK_RPAREN, "Se esperaba ')'");
        return expr_group(inner, l, c);
    }

    error_at_current(ps, "Expresión primaria esperada");
    // avanza para no quedar en loop
    advance_tok(ps);
    return expr_int(0, ps->prev.line, ps->prev.column);
}

static expr* parse_call_suffix(parser_t *ps, expr *callee){
    // llamada: '(' args? ')'
    if(!match(ps, TOK_LPAREN)) return callee;
    int l = ps->prev.line, c = ps->prev.column;
    expr *call = expr_call(callee, l, c);
    if(!check(ps, TOK_RPAREN)){
        do{
            expr *arg = parse_expression(ps);
            expr_args_push(call, arg);
        } while(match(ps, TOK_COMMA));
    }
    consume_or_err(ps, TOK_RPAREN, "Se esperaba ')' después de argumentos");
    return call;
}

// Ternario especial: ? { true: <e1> : false: <e2> }
static expr* parse_ternary_suffix(parser_t *ps, expr *cond){
    if(!match(ps, TOK_QUESTION)) return cond;
    consume_or_err(ps, TOK_LBRACE, "Se esperaba '{' después de '?'");
    consume_or_err(ps, TOK_TRUE, "Se esperaba 'true' en ternario");
    consume_or_err(ps, TOK_COLON, "Se esperaba ':' después de 'true'");
    expr *e_true = parse_expression(ps);
    consume_or_err(ps, TOK_COLON, "Se esperaba ':' separando ramas del ternario");
    consume_or_err(ps, TOK_FALSE, "Se esperaba 'false' en ternario");
    consume_or_err(ps, TOK_COLON, "Se esperaba ':' después de 'false'");
    expr *e_false = parse_expression(ps);
    consume_or_err(ps, TOK_RBRACE, "Se esperaba '}' al cerrar ternario");
    return expr_ternary(cond, e_true, e_false, cond->line, cond->col);
}

static expr* parse_unary(parser_t *ps){
    if(match(ps, TOK_NOT) || match(ps, TOK_MINUS)){
        token_type op_t = ps->prev.type;
        op_kind op = op_from_token(op_t);
        expr *right = parse_precedence(ps, PREC_UNARY);
        return expr_unary(op, right, ps->prev.line, ps->prev.column);
    }
    return parse_primary(ps);
}

static precedence precedence_of(token_type t){
    switch(t){
        case TOK_ASSIGN: case TOK_PLUS_ASSIGN: case TOK_MINUS_ASSIGN:
        case TOK_STAR_ASSIGN: case TOK_SLASH_ASSIGN: case TOK_PERCENT_ASSIGN:
            return PREC_ASSIGN;
        case TOK_OR:  return PREC_OR;
        case TOK_AND: return PREC_AND;
        case TOK_EQ: case TOK_NEQ: return PREC_EQUALITY;
        case TOK_LT: case TOK_LTE: case TOK_GT: case TOK_GTE: return PREC_COMPARE;
        case TOK_PLUS: case TOK_MINUS: return PREC_TERM;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return PREC_FACTOR;
        case TOK_LPAREN: return PREC_CALL;
        default: return PREC_LOWEST;
    }
}

static expr* parse_precedence(parser_t *ps, precedence prec){
    expr *left = parse_unary(ps);

    // Postfijos: llamada y ternario especial tienen mayor precedencia que binarios
    for(;;){
        // llamadas (encadenadas)
        if(precedence_of(ps->curr.type) == PREC_CALL){
            left = parse_call_suffix(ps, left);
            continue;
        }

        precedence nextp = precedence_of(ps->curr.type);
        if(nextp < prec) break;

        // asignación es right-assoc
        if(nextp == PREC_ASSIGN){
            // sólo permitimos lvalue = ... si left es IDENT
            if(left->kind != EXPR_IDENT){
                error_at_previous(ps, "El lado izquierdo de una asignación debe ser un identificador");
                // intenta consumir el operador para no buclear
                advance_tok(ps);
                // sigue parseando como si fuera binario para recuperar
                expr *rhs = parse_precedence(ps, PREC_ASSIGN);
                (void)rhs;
                break;
            }
            token_type op_t = ps->curr.type; advance_tok(ps);
            op_kind op = op_from_token(op_t);
            expr *rhs = parse_precedence(ps, PREC_ASSIGN);
            left = expr_assign(left->as.ident.name, op, rhs, left->line, left->col);
            continue;
        }

        // operadores binarios
        token_type op_t = ps->curr.type; advance_tok(ps);
        op_kind op = op_from_token(op_t);

        // definición de precedencia y asociatividad (izq-asoc)
        precedence rhs_prec = (nextp + 1);
        expr *right = parse_precedence(ps, rhs_prec);
        left = expr_binary(left, op, right, ps->prev.line, ps->prev.column);
    }

    // El ternario tiene menor precedencia que cualquier binario (como en C).
    // Sólo lo permitimos si estamos en un nivel de precedencia "alto" (<= OR).
    if (prec <= PREC_OR && check(ps, TOK_QUESTION)) {
        left = parse_ternary_suffix(ps, left);
    }

    return left;
}

static expr* parse_expression(parser_t *ps){
    return parse_precedence(ps, PREC_ASSIGN);
}

// ---------- sentencias ----------
static stmt* parse_return(parser_t *ps){
    int l=ps->prev.line, c=ps->prev.column;
    // `return` [expr]? ;
    if(!check(ps, TOK_SEMICOLON)){
        expr *e = parse_expression(ps);
        consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';' después de return");
        return stmt_return(e, l, c);
    } else {
        consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';' después de return");
        return stmt_return(NULL, l, c);
    }
}

static stmt* parse_if(parser_t *ps){
    int l=ps->prev.line, c=ps->prev.column;
    consume_or_err(ps, TOK_LPAREN, "Se esperaba '(' después de 'if'");
    expr *cond = parse_expression(ps);
    consume_or_err(ps, TOK_RPAREN, "Se esperaba ')'");
    stmt *thenB = parse_block_stmt(ps);
    stmt *elseB = NULL;
    if(match(ps, TOK_ELSE)){
        elseB = parse_block_stmt(ps);
    }
    return stmt_if(cond, thenB, elseB, l, c);
}

static stmt* parse_for(parser_t *ps){
    int l=ps->prev.line, c=ps->prev.column;
    consume_or_err(ps, TOK_LPAREN, "Se esperaba '(' después de 'for'");

    // Caso A: for (; ... )  → init vacío
    if (match(ps, TOK_SEMICOLON)) {
        expr *cond = NULL, *post = NULL;
        if (!check(ps, TOK_SEMICOLON)) cond = parse_expression(ps);
        consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';' en for");
        if (!check(ps, TOK_RPAREN))    post = parse_expression(ps);
        consume_or_err(ps, TOK_RPAREN, "Se esperaba ')' al cerrar for");
        stmt *body = parse_block_stmt(ps);
        return stmt_for_clike(/*init=*/NULL, cond, post, body, l, c);
    }

    // Caso B: for (variable ... ; ...)  o  for (const ... ; ...)
    if (check(ps, TOK_VARIABLE) || check(ps, TOK_CONST)) {
        decl *vd = parse_declaration(ps); // consume hasta ';'
        stmt *init_stmt = NULL;
        if (vd && vd->kind==DECL_VAR) {
            expr *assign = expr_assign(vd->as.var.name, OP_ASSIGN, vd->as.var.init, vd->as.var.line, vd->as.var.col);
            init_stmt = stmt_expr_stmt(assign, vd->as.var.line, vd->as.var.col);
            vd->as.var.init = NULL;
            free(vd->as.var.name);
            free(vd);
        }
        expr *cond = NULL, *post = NULL;
        if (!check(ps, TOK_SEMICOLON)) cond = parse_expression(ps);
        consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';' en for");
        if (!check(ps, TOK_RPAREN))    post = parse_expression(ps);
        consume_or_err(ps, TOK_RPAREN, "Se esperaba ')' al cerrar for");
        stmt *body = parse_block_stmt(ps);
        return stmt_for_clike(init_stmt, cond, post, body, l, c);
    }

    // Caso C / D: empieza con una expresión
    //   C-like:  for ( <expr> ; cond? ; post? ) { ... }
    //   while-like: for ( <expr> ) { ... }   (interpretamos <expr> como la condición)
    expr *first = parse_expression(ps);

    if (match(ps, TOK_RPAREN)) {
        // while-like
        stmt *body = parse_block_stmt(ps);
        return stmt_for_while(first, body, l, c);
    }

    // Debe ser C-like con init=expr-stmt
    consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';' en for");

    expr *cond = NULL, *post = NULL;
    if (!check(ps, TOK_SEMICOLON)) cond = parse_expression(ps);
    consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';' en for");
    if (!check(ps, TOK_RPAREN))    post = parse_expression(ps);
    consume_or_err(ps, TOK_RPAREN, "Se esperaba ')' al cerrar for");

    stmt *body = parse_block_stmt(ps);
    return stmt_for_clike(stmt_expr_stmt(first, l, c), cond, post, body, l, c);
}

static stmt* parse_statement(parser_t *ps){
    if(match(ps, TOK_RETURN))  return parse_return(ps);
    if(match(ps, TOK_BREAK))   { consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';'"); return stmt_break(ps->prev.line, ps->prev.column); }
    if(match(ps, TOK_CONTINUE)){ consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';'"); return stmt_continue(ps->prev.line, ps->prev.column); }
    if(match(ps, TOK_IF))      return parse_if(ps);
    if(match(ps, TOK_FOR))     return parse_for(ps);
    if(match(ps, TOK_LBRACE))  {
        // ya consumimos '{' => devolvemos bloque llenándolo
        stmt *blk = stmt_block(); // ubicaciones 0 por simplicidad
        while(!check(ps, TOK_RBRACE) && ps->curr.type != TOK_EOF){
            stmt *s = parse_statement(ps);
            stmt_block_push(blk, s);
        }
        consume_or_err(ps, TOK_RBRACE, "Se esperaba '}'");
        return blk;
    }

    // stmt expresión:
    expr *e = parse_expression(ps);
    consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';' al final de la sentencia");
    return stmt_expr_stmt(e, ps->prev.line, ps->prev.column);
}

static stmt* parse_block_stmt(parser_t *ps){
    consume_or_err(ps, TOK_LBRACE, "Se esperaba '{'");
    stmt *blk = stmt_block();
    while(!check(ps, TOK_RBRACE) && ps->curr.type != TOK_EOF){
        // En un bloque pueden venir declaraciones o sentencias
        if(check(ps, TOK_VARIABLE) || check(ps, TOK_CONST) || check(ps, TOK_Function)){
            decl *d = parse_declaration(ps);
            // Para este AST no hay stmt_decl, así que lo insertamos como expr-stmt equivalente cuando es var:
            if(d){
                if(d->kind==DECL_VAR){
                    expr *assign = expr_assign(d->as.var.name, OP_ASSIGN, d->as.var.init, d->as.var.line, d->as.var.col);
                    stmt_block_push(blk, stmt_expr_stmt(assign, d->as.var.line, d->as.var.col));
                    d->as.var.init=NULL; free(d->as.var.name); free(d); // consumido
                } else {
                    // Las funciones dentro de bloque no están en la gramática original.
                    // Si quisieras soportar anidadas, deberías ampliar AST.
                    error_at_previous(ps, "Declaración de función no permitida dentro de bloque");
                    decl_free(d);
                }
            }
        } else {
            stmt *s = parse_statement(ps);
            stmt_block_push(blk, s);
        }
    }
    consume_or_err(ps, TOK_RBRACE, "Se esperaba '}'");
    return blk;
}

// ---------- declaraciones ----------
static decl* parse_var_decl(parser_t *ps, bool is_const){
    // (variable|const) IDENT : tipo = expr ;
    int l = ps->prev.line, c = ps->prev.column;
    if(!match(ps, TOK_IDENT)){
        error_at_current(ps, "Se esperaba un identificador");
        return NULL;
    }
    char *name = dup_cstr(ps->prev.lexeme);
    consume_or_err(ps, TOK_COLON, "Se esperaba ':' después del nombre");
    type_spec t = parse_type_spec(ps);
    consume_or_err(ps, TOK_ASSIGN, "Se esperaba '=' en inicialización");
    expr *init = parse_expression(ps);
    consume_or_err(ps, TOK_SEMICOLON, "Se esperaba ';' al final de la declaración");

    decl *d = decl_var(name, is_const, t, init, l, c);
    free(name); // decl_var duplica
    return d;
}

static decl* parse_func_decl(parser_t *ps){
    // Function name (params?) -> type { body }
    int l=ps->prev.line, c=ps->prev.column;
    if(!match(ps, TOK_IDENT)){
        error_at_current(ps, "Se esperaba nombre de función");
        return NULL;
    }
    char *fname = dup_cstr(ps->prev.lexeme);
    consume_or_err(ps, TOK_LPAREN, "Se esperaba '('");

    decl *fn = decl_func(fname, type_make(TYPE_VOID), l, c);
    free(fname);

    if(!check(ps, TOK_RPAREN)){
        do{
            if(!match(ps, TOK_IDENT)){
                error_at_current(ps, "Se esperaba nombre de parámetro");
                break;
            }
            char *pname = dup_cstr(ps->prev.lexeme);
            consume_or_err(ps, TOK_COLON, "Se esperaba ':' después del parámetro");
            type_spec pt = parse_type_spec(ps);
            decl_func_param_push(fn, pname, pt);
            free(pname);
        } while(match(ps, TOK_COMMA));
    }
    consume_or_err(ps, TOK_RPAREN, "Se esperaba ')' al cerrar parámetros");
    consume_or_err(ps, TOK_ARROW,  "Se esperaba '->' antes del tipo de retorno");
    type_spec rt = parse_type_spec(ps);
    fn->as.func.ret_type = rt;

    stmt *body = parse_block_stmt(ps);
    decl_func_set_body(fn, body);
    return fn;
}

static decl* parse_declaration(parser_t *ps){
    if(match(ps, TOK_VARIABLE)) return parse_var_decl(ps, false);
    if(match(ps, TOK_CONST))    return parse_var_decl(ps, true);
    if(match(ps, TOK_Function)) return parse_func_decl(ps);

    // si no es declaración, tratamos de sentencia a nivel superior
    stmt *s = parse_statement(ps);
    // igual que en block: envolvemos stmt top-level inexistente en una pseudo declaración de var? No.
    // En la gramática original, toplevel son var/const y function; otras sentencias no deberían estar.
    // Pero para tolerancia, las ignoramos (o podríamos convertirlas en una función implícita).
    (void)s; // descartamos
    return NULL;
}

// ---------- programa ----------
program_ast parse_program(parser_t *ps){
    program_ast prog = program_make();
    while(ps->curr.type != TOK_EOF){
        decl *d = parse_declaration(ps);
        if(d) program_push_decl(&prog, d);
        if(ps->had_error) {
            synchronize(ps);
        }
    }
    return prog;
}

