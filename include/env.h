#ifndef ENV_H_
#define ENV_H_

#include "ast.h"
#include "value.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct var_entry {
    char *name;
    bool is_const;
    value_t val;
} var_entry;

typedef struct func_entry {
    char *name;
    func_decl *fn; // no copiamos el AST; lo referenciamos
} func_entry;

typedef struct env {
    struct env *parent;
    var_entry *vars;   size_t vars_count, vars_cap;
    func_entry *funcs; size_t funcs_count, funcs_cap;
} env_t;

env_t *env_new(env_t *parent);
void    env_free(env_t *e);

// variables
bool env_define_var(env_t *e, const char *name, bool is_const, value_t v);
bool env_set_var   (env_t *e, const char *name, value_t v);          // respeta const
bool env_get_var   (env_t *e, const char *name, value_t *out);

// funciones
bool env_define_func(env_t *e, const char *name, func_decl *fn);
func_decl *env_get_func(env_t *e, const char *name);

// builtin print
typedef value_t (*builtin_fn)(int argc, value_t *argv);
bool env_define_builtin(env_t *e, const char *name, builtin_fn fn);
builtin_fn env_get_builtin(env_t *e, const char *name);

#endif /* ENV_H_ */
