#ifndef TOKEN_H_
#define TOKEN_H_

#include <stddef.h> // size_t

// Tipos de token de Celer.
typedef enum {
    // Especiales
    TOK_ILLEGAL = 0,
    TOK_EOF,

    // Identificadores y literales
    TOK_IDENT,
    TOK_INT_LIT,
    TOK_FLOAT_LIT,
    TOK_STRING_LIT,
    TOK_BOOL_LIT,   // true/false

    // Palabras clave
    TOK_VARIABLE,   // variable
    TOK_CONST,      // const
    TOK_Function,   // Function (F mayúscula)
    TOK_RETURN,     // return
    TOK_FOR,        // for
    TOK_IF,         // if
    TOK_ELSE,       // else
    TOK_BREAK,      // break
    TOK_CONTINUE,   // continue
    TOK_INT,        // int
    TOK_BOOL,       // bool
    TOK_FLOAT,      // float
    TOK_STRING,     // string
    TOK_TRUE,       // true
    TOK_FALSE,      // false

    // Operadores aritméticos
    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_STAR,       // *
    TOK_SLASH,      // /
    TOK_PERCENT,    // %

    // Comparación
    TOK_EQ,         // ==
    TOK_NEQ,        // !=
    TOK_LT,         // <
    TOK_LTE,        // <=
    TOK_GT,         // >
    TOK_GTE,        // >=

    // Lógicos
    TOK_AND,        // &&
    TOK_OR,         // ||
    TOK_NOT,        // !

    // Asignación
    TOK_ASSIGN,     // =
    TOK_PLUS_ASSIGN,// +=
    TOK_MINUS_ASSIGN,// -=
    TOK_STAR_ASSIGN,// *=
    TOK_SLASH_ASSIGN,// /=
    TOK_PERCENT_ASSIGN,// %=

    // Delimitadores y otros
    TOK_LPAREN,     // (
    TOK_RPAREN,     // )
    TOK_LBRACE,     // {
    TOK_RBRACE,     // }
    TOK_LBRACKET,   // [
    TOK_RBRACKET,   // ]
    TOK_COMMA,      // ,
    TOK_SEMICOLON,  // ;
    TOK_COLON,      // :
    TOK_QUESTION,   // ?
    TOK_ARROW       // ->
} token_type;

// Estructura base de token.
typedef struct {
    token_type type;
    char *lexeme;       // Copia propia y null-terminated
    size_t length;      // Longitud del lexema (sin el '\0')
    int line;           // 1..N
    int column;         // 1..N (columna del primer carácter)
} token_t;

// --- API ---

// Crea un token duplicando [start, start+length).
token_t token_make(token_type type, const char *start, size_t length, int line, int column);

// Crea un token desde un C-string (usa strlen internamente).
token_t token_from_cstr(token_type type, const char *cstr, int line, int column);

// Copia profunda.
token_t token_copy(const token_t *src);

// Libera la memoria interna (lexeme) y pone el token en estado neutro.
void token_free(token_t *tok);

// Retorna un nombre legible del tipo (const char* estático).
const char *token_type_name(token_type t);

// Dada una cadena que cumple la regla de identificador, retorna el token de keyword o TOK_IDENT.
// Importante: reconoce "Function" (con F mayúscula); "function" se trata como identificador.
token_type token_keyword_lookup(const char *ident, size_t len);

#endif /* TOKEN_H_ */

