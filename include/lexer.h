#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>
#include "../include/token.h"

typedef struct {
    const char *src;
    size_t len;
    size_t pos;   // índice actual (0..len)
    int line;     // 1..N
    int col;      // 1..N
} lexer_t;

// Inicializa el lexer con un buffer y longitud.
void lexer_init(lexer_t *lx, const char *src, size_t len);

// Inicializa desde C-string (usa strlen).
void lexer_from_cstr(lexer_t *lx, const char *src);

// Obtiene el siguiente token (incluye EOF). En errores léxicos retorna TOK_ILLEGAL y avanza.
token_t lexer_next_token(lexer_t *lx);

#endif /* LEXER_H_ */
