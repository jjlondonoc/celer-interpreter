#include "../include/token.h"

#include <string.h>
#include <stdlib.h>

// strdup no es estándar, implementamos una versión local.
static char *celer_strdup_n(const char *s, size_t n) {
    char *p = (char*)malloc(n + 1u);
    if (!p) return NULL;
    if (n) memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

static char *celer_strdup(const char *s) {
    return celer_strdup_n(s, s ? strlen(s) : 0u);
}

// ---- Tabla de nombres de tipos ----
const char *token_type_name(token_type t) {
    switch (t) {
        case TOK_ILLEGAL: return "ILLEGAL";
        case TOK_EOF: return "EOF";
        case TOK_IDENT: return "IDENT";
        case TOK_INT_LIT: return "INT_LIT";
        case TOK_FLOAT_LIT: return "FLOAT_LIT";
        case TOK_STRING_LIT: return "STRING_LIT";
        case TOK_BOOL_LIT: return "BOOL_LIT";

        case TOK_VARIABLE: return "variable";
        case TOK_CONST: return "const";
        case TOK_Function: return "Function";
        case TOK_RETURN: return "return";
        case TOK_FOR: return "for";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_BREAK: return "break";
        case TOK_CONTINUE: return "continue";
        case TOK_INT: return "int";
        case TOK_BOOL: return "bool";
        case TOK_FLOAT: return "float";
        case TOK_STRING: return "string";
        case TOK_TRUE: return "true";
        case TOK_FALSE: return "false";

        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_PERCENT: return "%";

        case TOK_EQ: return "==";
        case TOK_NEQ: return "!=";
        case TOK_LT: return "<";
        case TOK_LTE: return "<=";
        case TOK_GT: return ">";
        case TOK_GTE: return ">=";

        case TOK_AND: return "&&";
        case TOK_OR: return "||";
        case TOK_NOT: return "!";

        case TOK_ASSIGN: return "=";
        case TOK_PLUS_ASSIGN: return "+=";
        case TOK_MINUS_ASSIGN: return "-=";
        case TOK_STAR_ASSIGN: return "*=";
        case TOK_SLASH_ASSIGN: return "/=";
        case TOK_PERCENT_ASSIGN: return "%=";

        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_LBRACKET: return "[";
        case TOK_RBRACKET: return "]";
        case TOK_COMMA: return ",";
        case TOK_SEMICOLON: return ";";
        case TOK_COLON: return ":";
        case TOK_QUESTION: return "?";
        case TOK_ARROW: return "->";
        default: return "?";
    }
}

// ---- Creación / copia / liberación ----
token_t token_make(token_type type, const char *start, size_t length, int line, int column) {
    token_t t;
    t.type = type;
    t.lexeme = celer_strdup_n(start ? start : "", length);
    t.length = t.lexeme ? length : 0u;
    t.line = line;
    t.column = column;
    return t;
}

token_t token_from_cstr(token_type type, const char *cstr, int line, int column) {
    token_t t;
    size_t n = cstr ? strlen(cstr) : 0u;
    t.type = type;
    t.lexeme = celer_strdup(cstr ? cstr : "");
    t.length = t.lexeme ? n : 0u;
    t.line = line;
    t.column = column;
    return t;
}

token_t token_copy(const token_t *src) {
    if (!src) {
        token_t z = { TOK_ILLEGAL, NULL, 0u, 0, 0 };
        return z;
    }
    token_t t;
    t.type = src->type;
    t.lexeme = celer_strdup(src->lexeme ? src->lexeme : "");
    t.length = src->lexeme ? src->length : 0u;
    t.line = src->line;
    t.column = src->column;
    return t;
}

void token_free(token_t *tok) {
    if (!tok) return;
    free(tok->lexeme);
    tok->lexeme = NULL;
    tok->length = 0u;
    tok->type = TOK_ILLEGAL;
    tok->line = tok->column = 0;
}

// ---- Keyword lookup ----
// Importante: sensible a mayúsculas.
// - "Function" (F mayúscula) es keyword. "function" NO lo es.
// - true/false los tratamos como keywords dedicadas (TOK_TRUE/TOK_FALSE)
typedef struct { const char *kw; token_type t; } kw_pair;

static const kw_pair KW_TABLE[] = {
    {"Function", TOK_Function},
    {"variable", TOK_VARIABLE},
    {"const",    TOK_CONST},
    {"return",   TOK_RETURN},
    {"for",      TOK_FOR},
    {"if",       TOK_IF},
    {"else",     TOK_ELSE},
    {"break",    TOK_BREAK},
    {"continue", TOK_CONTINUE},
    {"int",      TOK_INT},
    {"bool",     TOK_BOOL},
    {"float",    TOK_FLOAT},
    {"string",   TOK_STRING},
    {"true",     TOK_TRUE},
    {"false",    TOK_FALSE},
};

token_type token_keyword_lookup(const char *ident, size_t len) {
    if (!ident) return TOK_IDENT;
    for (size_t i = 0; i < sizeof(KW_TABLE)/sizeof(KW_TABLE[0]); ++i) {
        const char *k = KW_TABLE[i].kw;
        if (strlen(k) == len && strncmp(ident, k, len) == 0) {
            return KW_TABLE[i].t;
        }
    }
    return TOK_IDENT;
}

