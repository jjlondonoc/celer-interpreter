#ifndef EVAL_H_
#define EVAL_H_

#include "ast.h"
#include "env.h"
#include "value.h"
#include <stdbool.h>

typedef enum {
    SIG_NONE = 0,
    SIG_RETURN,
    SIG_BREAK,
    SIG_CONTINUE,
    SIG_RUNTIME_ERROR
} eval_signal;

typedef struct {
    eval_signal sig;
    value_t     value; // para return; en otros casos VAL_VOID
} eval_result;

eval_result eval_program(env_t *global, const program_ast *P);
// si existe Function main() -> void, la invoca automáticamente

#endif /* EVAL_H_ */
