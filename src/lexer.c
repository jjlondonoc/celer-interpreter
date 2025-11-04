#include "../include/lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Helpers de avance/peek
static int is_ident_start(int c) {
    return (c == '_' || isalpha((unsigned char)c));
}
static int is_ident_part(int c) {
    return (c == '_' || isalnum((unsigned char)c));
}
static int at_end(const lexer_t *lx) {
    return lx->pos >= lx->len;
}
static char peek(const lexer_t *lx) {
    return at_end(lx) ? '\0' : lx->src[lx->pos];
}
static char peek2(const lexer_t *lx) {
    return (lx->pos + 1u < lx->len) ? lx->src[lx->pos + 1u] : '\0';
}
static char peek3(const lexer_t *lx) {
    return (lx->pos + 2u < lx->len) ? lx->src[lx->pos + 2u] : '\0';
}
static char advance(lexer_t *lx) {
    if (at_end(lx)) return '\0';
    char c = lx->src[lx->pos++];
    if (c == '\n') {
        lx->line += 1;
        lx->col = 1;
    } else {
        lx->col += 1;
    }
    return c;
}
static int match(lexer_t *lx, char expected) {
    if (at_end(lx)) return 0;
    if (lx->src[lx->pos] != expected) return 0;
    advance(lx);
    return 1;
}

static void skip_spaces_and_linebreaks(lexer_t *lx) {
    for (;;) {
        char c = peek(lx);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lx);
            continue;
        }
        break;
    }
}

// Consume comentario de línea (*--) o bloque (/* ... */). Devuelve 1 si consumió algo y continúa.
// Si encuentra un bloque sin cierre, devuelve -1 (error).
static int try_skip_comment(lexer_t *lx) {
    char c = peek(lx);
    char c2 = peek2(lx);
    char c3 = peek3(lx);

    // Comentario de línea: *--
    if (c == '*' && c2 == '-' && c3 == '-') {
        // consumir *--
        advance(lx); advance(lx); advance(lx);
        // hasta fin de línea o EOF
        while (!at_end(lx) && peek(lx) != '\n') advance(lx);
        return 1;
    }

    // Comentario de bloque: /* ... */
    if (c == '/' && c2 == '*') {
        // consumir /*
        advance(lx); advance(lx);
        // avanzar hasta */
        while (!at_end(lx)) {
            char ch = advance(lx);
            if (ch == '*' && peek(lx) == '/') {
                advance(lx); // consumir '/'
                return 1;
            }
        }
        // EOF alcanzado sin cerrar
        return -1;
    }

    return 0;
}

static void skip_ignorable(lexer_t *lx, int *unterminated_block_comment) {
    *unterminated_block_comment = 0;
    for (;;) {
        skip_spaces_and_linebreaks(lx);
        int r = try_skip_comment(lx);
        if (r == 1) continue;
        if (r == -1) { *unterminated_block_comment = 1; return; }
        break;
    }
}

// Escanea un identificador/keyword
static token_t scan_ident_or_keyword(lexer_t *lx, int start_line, int start_col) {
    size_t start = lx->pos;
    while (is_ident_part(peek(lx))) advance(lx);
    size_t end = lx->pos;
    size_t len = end - start;
    // keyword lookup (sensible a mayúsculas)
    token_type t = token_keyword_lookup(&lx->src[start], len);
    return token_make(t, &lx->src[start], len, start_line, start_col);
}

// Escanea números (int o float sin notación científica)
static token_t scan_number(lexer_t *lx, int start_line, int start_col) {
    size_t start = lx->pos;
    // parte entera
    while (isdigit((unsigned char)peek(lx))) advance(lx);

    token_type t = TOK_INT_LIT;

    // parte decimal opcional: .<dígitos>
    if (peek(lx) == '.' && isdigit((unsigned char)peek2(lx))) {
        t = TOK_FLOAT_LIT;
        advance(lx); // consume '.'
        while (isdigit((unsigned char)peek(lx))) advance(lx);
    }

    size_t end = lx->pos;
    return token_make(t, &lx->src[start], end - start, start_line, start_col);
}

// Escanea string con escapes básicos. Conserva el lexema con comillas.
// Si no cierra, emite TOK_ILLEGAL con mensaje.
static token_t scan_string(lexer_t *lx, int start_line, int start_col) {
    size_t start = lx->pos - 1; // incluye la comilla inicial
    int closed = 0;

    while (!at_end(lx)) {
        char ch = advance(lx);
        if (ch == '\"') { closed = 1; break; }
        if (ch == '\\') {
            // escape: consumir el siguiente si existe
            if (!at_end(lx)) advance(lx);
        }
        // las nuevas líneas dentro de string son válidas sólo si van escapadas,
        // aquí permitimos \n (el real newline ya ha incrementado línea/col).
    }

    if (!closed) {
        // crear un token ILLEGAL con mensaje
        return token_from_cstr(TOK_ILLEGAL, "Unterminated string literal", start_line, start_col);
    }
    size_t end = lx->pos;
    return token_make(TOK_STRING_LIT, &lx->src[start], end - start, start_line, start_col);
}

void lexer_init(lexer_t *lx, const char *src, size_t len) {
    lx->src = src ? src : "";
    lx->len = src ? len : 0u;
    lx->pos = 0u;
    lx->line = 1;
    lx->col = 1;
}
void lexer_from_cstr(lexer_t *lx, const char *src) {
    lexer_init(lx, src, src ? strlen(src) : 0u);
}

static token_t make_single(token_type t, const char *lex, int line, int col) {
    return token_from_cstr(t, lex, line, col);
}

token_t lexer_next_token(lexer_t *lx) {
    if (!lx) {
        return token_from_cstr(TOK_ILLEGAL, "NULL lexer", 0, 0);
    }

    for (;;) {
        int unterminated = 0;
        skip_ignorable(lx, &unterminated);
        if (unterminated) {
            return token_from_cstr(TOK_ILLEGAL, "Unterminated block comment", lx->line, lx->col);
        }
        break;
    }

    if (at_end(lx)) {
        return token_from_cstr(TOK_EOF, "", lx->line, lx->col);
    }

    int start_line = lx->line;
    int start_col  = lx->col;
    char c = advance(lx);

    // Identificadores / keywords
    if (is_ident_start(c)) {
        // retrocede 1 (porque advance ya lo consumió) y usa escáner
        lx->pos--; lx->col--;
        return scan_ident_or_keyword(lx, start_line, start_col);
    }

    // Números
    if (isdigit((unsigned char)c)) {
        lx->pos--; lx->col--;
        return scan_number(lx, start_line, start_col);
    }

    // Strings
    if (c == '\"') {
        return scan_string(lx, start_line, start_col);
    }

    // Operadores de 2 caracteres y lógicos, asignaciones compuestas, flecha
    switch (c) {
        case '+':
            if (match(lx, '=')) return make_single(TOK_PLUS_ASSIGN, "+=", start_line, start_col);
            return make_single(TOK_PLUS, "+", start_line, start_col);
        case '-':
            if (match(lx, '=')) return make_single(TOK_MINUS_ASSIGN, "-=", start_line, start_col);
            if (match(lx, '>')) return make_single(TOK_ARROW, "->", start_line, start_col);
            return make_single(TOK_MINUS, "-", start_line, start_col);
        case '*':
            // Ojo: comentario de línea "*--" ya fue manejado en skip_ignorable.
            if (match(lx, '=')) return make_single(TOK_STAR_ASSIGN, "*=", start_line, start_col);
            return make_single(TOK_STAR, "*", start_line, start_col);
        case '/':
            if (match(lx, '=')) return make_single(TOK_SLASH_ASSIGN, "/=", start_line, start_col);
            // "/*" ya se trató en skip_ignorable; si llega aquí es "/"
            return make_single(TOK_SLASH, "/", start_line, start_col);
        case '%':
            if (match(lx, '=')) return make_single(TOK_PERCENT_ASSIGN, "%=", start_line, start_col);
            return make_single(TOK_PERCENT, "%", start_line, start_col);
        case '=':
            if (match(lx, '=')) return make_single(TOK_EQ, "==", start_line, start_col);
            return make_single(TOK_ASSIGN, "=", start_line, start_col);
        case '!':
            if (match(lx, '=')) return make_single(TOK_NEQ, "!=", start_line, start_col);
            return make_single(TOK_NOT, "!", start_line, start_col);
        case '<':
            if (match(lx, '=')) return make_single(TOK_LTE, "<=", start_line, start_col);
            return make_single(TOK_LT, "<", start_line, start_col);
        case '>':
            if (match(lx, '=')) return make_single(TOK_GTE, ">=", start_line, start_col);
            return make_single(TOK_GT, ">", start_line, start_col);
        case '&':
            if (match(lx, '&')) return make_single(TOK_AND, "&&", start_line, start_col);
            break; // ilegal si '&' suelto
        case '|':
            if (match(lx, '|')) return make_single(TOK_OR, "||", start_line, start_col);
            break; // ilegal si '|' suelto

        // Delimitadores
        case '(' : return make_single(TOK_LPAREN, "(", start_line, start_col);
        case ')' : return make_single(TOK_RPAREN, ")", start_line, start_col);
        case '{' : return make_single(TOK_LBRACE, "{", start_line, start_col);
        case '}' : return make_single(TOK_RBRACE, "}", start_line, start_col);
        case '[' : return make_single(TOK_LBRACKET, "[", start_line, start_col);
        case ']' : return make_single(TOK_RBRACKET, "]", start_line, start_col);
        case ',' : return make_single(TOK_COMMA, ",", start_line, start_col);
        case ';' : return make_single(TOK_SEMICOLON, ";", start_line, start_col);
        case ':' : return make_single(TOK_COLON, ":", start_line, start_col);
        case '?' : return make_single(TOK_QUESTION, "?", start_line, start_col);
        default: break;
    }

    // Si nada matcheó, es ilegal
    char buf[2] = { c, '\0' };
    return token_from_cstr(TOK_ILLEGAL, buf, start_line, start_col);
}
