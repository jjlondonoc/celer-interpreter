#include "../include/env.h"
#include <stdlib.h>
#include <string.h>

static char *dup_cstr(const char *s){
    if(!s){ char *z=(char*)malloc(1); if(z) z[0]='\0'; return z; }
    size_t n=strlen(s); char *p=(char*)malloc(n+1); if(!p) return NULL; memcpy(p,s,n+1); return p;
}

typedef struct builtin_entry {
    char *name;
    builtin_fn fn;
} builtin_entry;

typedef struct env_priv {
    builtin_entry *b; size_t bc, bp;
} env_priv;

env_t *env_new(env_t *parent){
    env_t *e=(env_t*)calloc(1,sizeof(env_t));
    e->parent=parent;
    return e;
}
static void free_vars(env_t *e){
    for(size_t i=0;i<e->vars_count;i++){
        free(e->vars[i].name);
        value_free(&e->vars[i].val);
    }
    free(e->vars);
}
static void free_funcs(env_t *e){
    for(size_t i=0;i<e->funcs_count;i++){
        free(e->funcs[i].name);
        // no liberamos e->funcs[i].fn (vive en AST)
    }
    free(e->funcs);
}
void env_free(env_t *e){
    if(!e) return;
    free_vars(e);
    free_funcs(e);
    // builtin table está colgando de e->funcs? No; hacemos un “priv” escondido opcional:
    // Para minimizar, no mantenemos estado extra aquí (simplificado).
    free(e);
}

static int find_var(env_t *e, const char *name, env_t **out_env, size_t *out_idx){
    for(env_t *cur=e; cur; cur=cur->parent){
        for(size_t i=0;i<cur->vars_count;i++){
            if(strcmp(cur->vars[i].name, name)==0){ if(out_env) *out_env=cur; if(out_idx) *out_idx=i; return 1; }
        }
    }
    return 0;
}
bool env_define_var(env_t *e, const char *name, bool is_const, value_t v){
    // sombreado permitido
    if(e->vars_count==e->vars_cap){ size_t nc=e->vars_cap?e->vars_cap*2u:8u; e->vars=(var_entry*)realloc(e->vars, nc*sizeof(var_entry)); e->vars_cap=nc; }
    e->vars[e->vars_count].name=dup_cstr(name);
    e->vars[e->vars_count].is_const=is_const;
    e->vars[e->vars_count].val=value_copy(&v);
    e->vars_count++;
    return true;
}
bool env_set_var(env_t *e, const char *name, value_t v){
    env_t *where=NULL; size_t idx=0;
    if(!find_var(e,name,&where,&idx)) return false;
    if(where->vars[idx].is_const) return false;
    value_free(&where->vars[idx].val);
    where->vars[idx].val=value_copy(&v);
    return true;
}
bool env_get_var(env_t *e, const char *name, value_t *out){
    env_t *where=NULL; size_t idx=0;
    if(!find_var(e,name,&where,&idx)) return false;
    if(out) *out = value_copy(&where->vars[idx].val);
    return true;
}

bool env_define_func(env_t *e, const char *name, func_decl *fn){
    if(e->funcs_count==e->funcs_cap){ size_t nc=e->funcs_cap?e->funcs_cap*2u:8u; e->funcs=(func_entry*)realloc(e->funcs, nc*sizeof(func_entry)); e->funcs_cap=nc; }
    e->funcs[e->funcs_count].name=dup_cstr(name);
    e->funcs[e->funcs_count].fn=fn;
    e->funcs_count++;
    return true;
}
func_decl *env_get_func(env_t *e, const char *name){
    for(env_t *cur=e; cur; cur=cur->parent){
        // buscar de la más nueva a la más vieja
        for(size_t i=cur->funcs_count; i>0; i--){
            size_t j = i - 1;
            if(strcmp(cur->funcs[j].name, name)==0) return cur->funcs[j].fn;
        }
    }
    return NULL;
}

/*func_decl *env_get_func(env_t *e, const char *name){
    for(env_t *cur=e; cur; cur=cur->parent){
        for(size_t i=0;i<cur->funcs_count;i++){
            if(strcmp(cur->funcs[i].name, name)==0) return cur->funcs[i].fn;
        }
    }
    return NULL;
}*/

// --- builtins muy simple: guardamos en variables con prefijo especial ---
typedef struct builtin_store {
    char *name;
    builtin_fn fn;
} builtin_store;
static builtin_store *g_bstore=NULL; static size_t g_bc=0,g_bp=0;

bool env_define_builtin(env_t *e, const char *name, builtin_fn fn){
    (void)e; // global estático simple
    if(g_bc==g_bp){ size_t nc=g_bp?g_bp*2u:8u; g_bstore=(builtin_store*)realloc(g_bstore, nc*sizeof(builtin_store)); g_bp=nc; }
    g_bstore[g_bc].name=dup_cstr(name);
    g_bstore[g_bc].fn=fn;
    g_bc++;
    return true;
}
builtin_fn env_get_builtin(env_t *e, const char *name){
    (void)e;
    for(size_t i=0;i<g_bc;i++) if(strcmp(g_bstore[i].name,name)==0) return g_bstore[i].fn;
    return NULL;
}
