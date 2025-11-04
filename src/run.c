#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/env.h"
#include "../include/eval.h"

static char *read_file(const char *path, size_t *out_len){
    FILE *f = fopen(path, "rb"); if(!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if(n < 0){ fclose(f); return NULL; }
    char *buf = (char*)malloc((size_t)n + 1);
    if(!buf){ fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[rd] = '\0';
    if(out_len) *out_len = rd;
    return buf;
}

static char *read_stdin_all(void){
    size_t cap=4096, n=0; char *buf=(char*)malloc(cap);
    int ch;
    while((ch=fgetc(stdin))!=EOF){
        if(n+1>=cap){ cap*=2; buf=(char*)realloc(buf,cap); }
        buf[n++]=(char)ch;
    }
    buf[n]='\0';
    return buf;
}

int main(int argc, char **argv){
    char *source=NULL; size_t slen=0;

    if(argc>=2){
        source = read_file(argv[1], &slen);
        if(!source){ fprintf(stderr,"No pude leer %s\n", argv[1]); return 1; }
    } else {
        printf("Escribe tu programa Celer completo y presiona Ctrl+D (Unix) / Ctrl+Z (Windows) para ejecutar:\n");
        source = read_stdin_all();
    }

    lexer_t lx; lexer_init(&lx, source, slen? slen: strlen(source));
    parser_t ps; parser_init(&ps, &lx);
    program_ast P = parse_program(&ps);

    const parse_error_list *errs = parser_errors(&ps);
    if(errs->count){
        fprintf(stderr,"Errores de parseo: %zu\n", errs->count);
        for(size_t i=0;i<errs->count;i++){
            fprintf(stderr," @%d:%d %s\n", errs->items[i].line, errs->items[i].col, errs->items[i].message);
        }
        program_free(&P); parser_dispose(&ps); free(source);
        return 2;
    }

    env_t *global = env_new(NULL);
    (void)eval_program(global, &P);

    // Limpieza
    env_free(global);
    program_free(&P);
    parser_dispose(&ps);
    free(source);
    return 0;
}
