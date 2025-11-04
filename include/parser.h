#ifndef CELER_PARSER_H_
#define CELER_PARSER_H_

#include <stddef.h>
#include <stdbool.h>
#include "lexer.h"
#include "ast.h"

typedef struct {
    int line, col;
    char *message; // heap
} parse_error;

typedef struct {
    parse_error *items;
    size_t count, cap;
} parse_error_list;

typedef struct {
    lexer_t *lx;
    token_t curr; // lookahead actual
    token_t prev; // último consumido (para ubicaciones)
    bool had_error;

    parse_error_list errors;
} parser_t;

// API
void parser_init(parser_t *ps, lexer_t *lx);
void parser_dispose(parser_t *ps); // libera tokens pendientes y messages

program_ast parse_program(parser_t *ps);

const parse_error_list* parser_errors(const parser_t *ps);

#endif /* PARSER_H_ */
