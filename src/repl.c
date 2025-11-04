#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/env.h"
#include "../include/eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Trim izquierdo
static const char* lstrip(const char *s){
    while (*s==' ' || *s=='\t' || *s=='\r') ++s;
    return s;
}

static int starts_with_kw(const char *s, const char *kw){
    s = lstrip(s);
    size_t n = strlen(kw);
    if (strncmp(s, kw, n) != 0) return 0;
    char c = s[n];
    return (c==' '||c=='\t'||c=='\r'||c=='\n'||c=='('||c==':'||c=='\0');
}

// Lee un chunk hasta una línea que sea exactamente ";;\n" (o ";;\r\n").
// Retorna buffer heap con '\0'. Devuelve NULL en EOF inmediato.
static char* read_chunk(void){
    size_t cap = 1024, len = 0;
    char *buf = (char*)malloc(cap);
    if(!buf) return NULL;

    int first = 1;
    for(;;){
        fputs(first? "celer> " : "....> ", stdout);
        fflush(stdout);
        first = 0;

        char line[4096];
        if(!fgets(line, sizeof(line), stdin)){
            // EOF
            if(len==0){ free(buf); return NULL; }
            break;
        }
        if(strcmp(line, ";;\n")==0 || strcmp(line, ";;\r\n")==0){
            break;
        }
        if(strcmp(line, ":quit\n")==0 || strcmp(line, ":quit\r\n")==0){
            free(buf);
            return NULL;
        }
        if(strcmp(line, ":help\n")==0 || strcmp(line, ":help\r\n")==0){
            puts("Escribe declaraciones o sentencias Celer. Termina el bloque con ';;' en una linea.");
            puts("Comandos: :quit (salir), :help (esta ayuda)");
            continue;
        }

        size_t l = strlen(line);
        if(len + l + 1 >= cap){
            cap = (cap*2u > len+l+1 ? cap*2u : len+l+1);
            buf = (char*)realloc(buf, cap);
            if(!buf) return NULL;
        }
        memcpy(buf+len, line, l);
        len += l;
        buf[len] = '\0';
    }
    return buf;
}

// Ejecuta un "programa" en el entorno dado
static int run_program_in_env(env_t *global, const char *src){
    lexer_t lx; lexer_from_cstr(&lx, src);
    parser_t ps; parser_init(&ps, &lx);
    program_ast P = parse_program(&ps);

    const parse_error_list *errs = parser_errors(&ps);
    if(errs->count){
        fprintf(stderr,"Errores de parseo: %zu\n", errs->count);
        for(size_t i=0;i<errs->count;i++){
            fprintf(stderr," @%d:%d %s\n", errs->items[i].line, errs->items[i].col, errs->items[i].message);
        }
        program_free(&P); parser_dispose(&ps);
        return 1;
    }

    // Usa el evaluator para cargar vars/funcs y ejecutar main() si existe
    (void)eval_program(global, &P);

    program_free(&P);
    parser_dispose(&ps);
    return 0;
}

int main(void){
    printf("Celer REPL\n");
    printf("Termina cada bloque con ';;'. Comandos: :help, :quit\n");
#ifdef _WIN32
    printf("(Sugerencia Windows: ejecuta 'chcp 65001' para ver UTF-8 correctamente)\n");
#endif

    env_t *global = env_new(NULL);

    for(;;){
        char *chunk = read_chunk();
        if(!chunk) break; // :quit o EOF

        const char *trim = lstrip(chunk);
        int is_decl = starts_with_kw(trim, "variable") || starts_with_kw(trim, "const") || starts_with_kw(trim, "Function");

        int rc = 0;
        if(is_decl){
            // Compilar/registrar declaraciones tal cual
            rc = run_program_in_env(global, chunk);
        } else {
            // Envolver sentencias/expresiones en una función main() temporal
            const char *pre = "Function main() -> void {\n";
            const char *suf = "\n}\n";
            size_t n = strlen(pre) + strlen(chunk) + strlen(suf) + 1;
            char *wrapped = (char*)malloc(n);
            if(!wrapped){ free(chunk); break; }
            strcpy(wrapped, pre);
            strcat(wrapped, chunk);
            strcat(wrapped, suf);

            rc = run_program_in_env(global, wrapped);
            free(wrapped);
        }

        free(chunk);
        if(rc != 0){
            // continúa el REPL aún con errores
        }
    }

    env_free(global);
    puts("Adiós!");
    return 0;
}

