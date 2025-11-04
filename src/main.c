#include <stdio.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"

static const char *program_src =
    "*-- Programa demo Celer\n"
    "variable i : int = 0;\n"
    "const msg : string = \"hola\\n\";\n"
    "Function suma(a : int, b : int) -> int {\n"
    "  return a + b;\n"
    "}\n"
    "Function main() -> void {\n"
    "  if (true && false || !false) { i += 1; } else { i = i - 1; }\n"
    "  for (variable k : int = 0; k < 3; k = k + 1) { }\n"
    "  for (i < 10) { i = i + 1; }\n"
    "  i = (10 + 2) * 3 == 36 ? { true: 1 : false: 0 };\n"
    "  return;\n"
    "}\n";

int main(void){
    lexer_t lx; lexer_from_cstr(&lx, program_src);
    parser_t ps; parser_init(&ps, &lx);

    program_ast P = parse_program(&ps);

    printf("==== AST DUMP ====\n");
    ast_print_program(&P);

    const parse_error_list *errs = parser_errors(&ps);
    if(errs->count){
        printf("\n==== PARSE ERRORS (%zu) ====\n", errs->count);
        for(size_t i=0;i<errs->count;i++){
            printf(" @%d:%d  %s\n", errs->items[i].line, errs->items[i].col, errs->items[i].message);
        }
    } else {
        printf("\n(No parse errors)\n");
    }

    program_free(&P);
    parser_dispose(&ps);
    return 0;
}
