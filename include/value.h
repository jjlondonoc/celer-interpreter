#ifndef VALUE_H_
#define VALUE_H_
#include <stdbool.h>

typedef enum {
    VAL_VOID = 0,
    VAL_INT,
    VAL_BOOL,
    VAL_FLOAT,
    VAL_STRING
} value_kind;

typedef struct {
    value_kind kind;
    union {
        long long   i;
        double      f;
        bool        b;
        char       *s; // heap (propiedad del valor)
    } as;
} value_t;

// constructores
value_t v_void(void);
value_t v_int(long long x);
value_t v_float(double x);
value_t v_bool(bool x);
value_t v_string(const char *s);

// utilidades
void value_free(value_t *v);
value_t value_copy(const value_t *v);
const char *value_kind_name(value_kind k);

// coerciones sencillas
value_t value_to_bool(const value_t *v);   // 0/false/empty -> false
value_t value_to_float(const value_t *v);  // int->float, bool->0/1, string no permitido
value_t value_to_int(const value_t *v);    // float trunc, bool->0/1, string no permitido
char   *value_to_cstr(const value_t *v);   // genera string (heap) para print

// operaciones (devuelven VAL_VOID con s==NULL si error de tipo)
value_t value_add(const value_t *a, const value_t *b); // soporta int/float; string + string concat
value_t value_sub(const value_t *a, const value_t *b);
value_t value_mul(const value_t *a, const value_t *b);
value_t value_div(const value_t *a, const value_t *b);
value_t value_mod(const value_t *a, const value_t *b);
value_t value_eq (const value_t *a, const value_t *b);
value_t value_neq(const value_t *a, const value_t *b);
value_t value_lt (const value_t *a, const value_t *b);
value_t value_lte(const value_t *a, const value_t *b);
value_t value_gt (const value_t *a, const value_t *b);
value_t value_gte(const value_t *a, const value_t *b);
value_t value_and(const value_t *a, const value_t *b);
value_t value_or (const value_t *a, const value_t *b);
value_t value_not(const value_t *a);

#endif /* VALUE_H_ */