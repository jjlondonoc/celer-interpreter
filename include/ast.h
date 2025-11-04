#ifndef CELER_AST_H_
#define CELER_AST_H_

#include <stddef.h>
#include <stdbool.h>

// -------------------- Tipos de Celer --------------------
typedef enum {
    TYPE_VOID = 0,
    TYPE_INT,
    TYPE_BOOL,
    TYPE_FLOAT,
    TYPE_STRING
} type_kind;

typedef struct {
    type_kind kind;
} type_spec;

// Helpers
static inline type_spec type_make(type_kind k) { type_spec t; t.kind = k; return t; }

// -------------------- Forward decls --------------------
typedef struct expr expr;
typedef struct stmt stmt;
typedef struct decl decl;

// -------------------- Expr kinds -----------------------
typedef enum {
    EXPR_IDENT,
    EXPR_INT_LIT,
    EXPR_FLOAT_LIT,
    EXPR_BOOL_LIT,
    EXPR_STRING_LIT,

    EXPR_UNARY,        // op expr
    EXPR_BINARY,       // left op right
    EXPR_ASSIGN,       // name op= value  (op puede ser =, +=, etc.)
    EXPR_GROUPING,     // (expr)
    EXPR_TERNARY,      // ¿cond? { true: a : false: b }
    EXPR_CALL          // callee(args...)
} expr_kind;

// Operadores soportados
typedef enum {
    OP_NONE = 0,
    OP_ASSIGN, OP_PLUS_ASSIGN, OP_MINUS_ASSIGN, OP_STAR_ASSIGN, OP_SLASH_ASSIGN, OP_PERCENT_ASSIGN,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NEQ, OP_LT, OP_LTE, OP_GT, OP_GTE,
    OP_AND, OP_OR, OP_NOT
} op_kind;

// Vectores simples
typedef struct {
    expr **items;
    size_t count;
    size_t cap;
} expr_vec;

typedef struct {
    stmt **items;
    size_t count;
    size_t cap;
} stmt_vec;

// -------------------- Expresiones ----------------------
struct expr {
    expr_kind kind;
    int line, col; // ubicación aproximada

    union {
        struct { char *name; } ident;

        struct { long long value; } int_lit;
        struct { double value; } float_lit;
        struct { bool value; } bool_lit;
        struct { char *text; } string_lit;

        struct { op_kind op; expr *right; } unary;
        struct { expr *left; op_kind op; expr *right; } binary;

        struct { char *name; op_kind op; expr *value; } assign;  // lvalue =/+= ... value

        struct { expr *inner; } grouping;

        struct { expr *cond; expr *when_true; expr *when_false; } ternary;

        struct { expr *callee; expr_vec args; } call; // callee puede ser IDENT u otra expr
    } as;
};

// -------------------- Sentencias -----------------------
typedef enum {
    STMT_EXPR,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_BLOCK,
    STMT_IF,
    STMT_FOR_WHILELIKE, // for (cond) { ... }
    STMT_FOR_CLIKE      // for (init; cond; post) { ... }
} stmt_kind;

typedef struct {
    char *name;
    type_spec type;
} param_decl;

typedef struct {
    param_decl *items;
    size_t count;
    size_t cap;
} param_vec;

struct stmt {
    stmt_kind kind;
    int line, col;

    union {
        struct { expr *value; } expr_stmt;

        struct { expr *value; } ret;

        struct { /* vacío */ int _; } brk;
        struct { /* vacío */ int _2; } cont;

        struct { stmt_vec stmts; } block;

        struct { expr *cond; stmt *then_branch; stmt *else_branch; } if_stmt;

        struct { expr *cond; stmt *body; } for_while;

        struct { 
            stmt *init;    // puede ser NULL
            expr *cond;    // puede ser NULL
            expr *post;    // puede ser NULL (se ejecuta tras cada iteración)
            stmt *body;
        } for_clike;
    } as;
};

// -------------------- Declaraciones --------------------
typedef enum {
    DECL_VAR,
    DECL_FUNC
} decl_kind;

typedef struct {
    char *name;
    bool is_const;
    type_spec type;
    expr *init; // puede ser NULL
    int line, col;
} var_decl;

typedef struct {
    char *name;
    param_vec params;
    type_spec ret_type;
    stmt *body; // un bloque obligatorio
    int line, col;
} func_decl;

struct decl {
    decl_kind kind;
    union {
        var_decl var;
        func_decl func;
    } as;
};

typedef struct {
    decl **items;
    size_t count;
    size_t cap;
} decl_vec;

typedef struct {
    decl_vec decls;
} program_ast;

// --------------- API: constructores y utils ------------
expr *expr_ident(const char *name, int line, int col);
expr *expr_int(long long v, int line, int col);
expr *expr_float(double v, int line, int col);
expr *expr_bool(bool v, int line, int col);
expr *expr_string(const char *text, int line, int col);
expr *expr_unary(op_kind op, expr *right, int line, int col);
expr *expr_binary(expr *left, op_kind op, expr *right, int line, int col);
expr *expr_assign(const char *name, op_kind op, expr *value, int line, int col);
expr *expr_group(expr *inner, int line, int col);
expr *expr_ternary(expr *cond, expr *when_true, expr *when_false, int line, int col);
expr *expr_call(expr *callee, int line, int col);

void expr_args_push(expr *call_expr, expr *arg);

stmt *stmt_expr_stmt(expr *e, int line, int col);
stmt *stmt_return(expr *e, int line, int col);
stmt *stmt_break(int line, int col);
stmt *stmt_continue(int line, int col);
stmt *stmt_block(void);
void  stmt_block_push(stmt *block, stmt *s);
stmt *stmt_if(expr *cond, stmt *then_branch, stmt *else_branch, int line, int col);
stmt *stmt_for_while(expr *cond, stmt *body, int line, int col);
stmt *stmt_for_clike(stmt *init, expr *cond, expr *post, stmt *body, int line, int col);

decl *decl_var(const char *name, bool is_const, type_spec t, expr *init, int line, int col);
decl *decl_func(const char *name, type_spec ret_type, int line, int col);
void  decl_func_param_push(decl *fn, const char *name, type_spec t);
void  decl_func_set_body(decl *fn, stmt *body);

program_ast program_make(void);
void program_push_decl(program_ast *p, decl *d);

// Liberación profunda
void expr_free(expr *e);
void stmt_free(stmt *s);
void decl_free(decl *d);
void program_free(program_ast *p);

// Pretty print (para depuración)
void ast_print_program(const program_ast *p);

#endif /* CELER_AST_H */
