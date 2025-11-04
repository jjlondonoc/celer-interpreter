#include "../include/value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static char *dup_cstr(const char *s){
    if(!s){ char *z=(char*)malloc(1); if(z) z[0]='\0'; return z; }
    size_t n=strlen(s); char *p=(char*)malloc(n+1); if(!p) return NULL; memcpy(p,s,n+1); return p;
}

value_t v_void(void){ value_t v; v.kind=VAL_VOID; v.as.s=NULL; return v; }
value_t v_int(long long x){ value_t v; v.kind=VAL_INT; v.as.i=x; return v; }
value_t v_float(double x){ value_t v; v.kind=VAL_FLOAT; v.as.f=x; return v; }
value_t v_bool(bool x){ value_t v; v.kind=VAL_BOOL; v.as.b=x; return v; }
value_t v_string(const char *s){ value_t v; v.kind=VAL_STRING; v.as.s=dup_cstr(s?s:""); return v; }

void value_free(value_t *v){
    if(!v) return;
    if(v->kind==VAL_STRING) free(v->as.s);
    v->kind=VAL_VOID; v->as.s=NULL;
}
value_t value_copy(const value_t *v){
    if(!v) return v_void();
    if(v->kind==VAL_STRING) return v_string(v->as.s);
    return *v;
}
const char *value_kind_name(value_kind k){
    switch(k){
        case VAL_VOID: return "void";
        case VAL_INT: return "int";
        case VAL_BOOL: return "bool";
        case VAL_FLOAT: return "float";
        case VAL_STRING: return "string";
        default: return "?";
    }
}

value_t value_to_bool(const value_t *v){
    switch(v->kind){
        case VAL_BOOL:  return v_bool(v->as.b);
        case VAL_INT:   return v_bool(v->as.i!=0);
        case VAL_FLOAT: return v_bool(fabs(v->as.f) > 1e-12);
        case VAL_STRING:return v_bool(v->as.s && v->as.s[0]!='\0');
        default:        return v_bool(false);
    }
}
value_t value_to_float(const value_t *v){
    switch(v->kind){
        case VAL_FLOAT: return v_float(v->as.f);
        case VAL_INT:   return v_float((double)v->as.i);
        case VAL_BOOL:  return v_float(v->as.b?1.0:0.0);
        default:        return v_void();
    }
}
value_t value_to_int(const value_t *v){
    switch(v->kind){
        case VAL_INT:   return v_int(v->as.i);
        case VAL_BOOL:  return v_int(v->as.b?1:0);
        case VAL_FLOAT: return v_int((long long)v->as.f);
        default:        return v_void();
    }
}
char *value_to_cstr(const value_t *v){
    char buf[64];
    switch(v->kind){
        case VAL_VOID: return dup_cstr("void");
        case VAL_BOOL: return dup_cstr(v->as.b?"true":"false");
        case VAL_INT:  snprintf(buf,sizeof(buf),"%lld", v->as.i); return dup_cstr(buf);
        case VAL_FLOAT:snprintf(buf,sizeof(buf),"%g", v->as.f); return dup_cstr(buf);
        case VAL_STRING: return dup_cstr(v->as.s?v->as.s:"");
        default: return dup_cstr("?");
    }
}

static int both_floaty(const value_t *a, const value_t *b){ return a->kind==VAL_FLOAT || b->kind==VAL_FLOAT; }

value_t value_add(const value_t *a, const value_t *b){
    if(a->kind==VAL_STRING && b->kind==VAL_STRING){
        size_t na=strlen(a->as.s), nb=strlen(b->as.s);
        char *s=(char*)malloc(na+nb+1);
        if(!s) return v_void();
        memcpy(s,a->as.s,na); memcpy(s+na,b->as.s,nb); s[na+nb]='\0';
        value_t v=v_string(s); free(s); return v;
    }
    if(both_floaty(a,b)) return v_float((a->kind==VAL_FLOAT? a->as.f:(double)a->as.i) + (b->kind==VAL_FLOAT? b->as.f:(double)b->as.i));
    if(a->kind==VAL_INT && b->kind==VAL_INT) return v_int(a->as.i + b->as.i);
    return v_void();
}
value_t value_sub(const value_t *a, const value_t *b){
    if(both_floaty(a,b)) return v_float((a->kind==VAL_FLOAT? a->as.f:(double)a->as.i) - (b->kind==VAL_FLOAT? b->as.f:(double)b->as.i));
    if(a->kind==VAL_INT && b->kind==VAL_INT) return v_int(a->as.i - b->as.i);
    return v_void();
}
value_t value_mul(const value_t *a, const value_t *b){
    if(both_floaty(a,b)) return v_float((a->kind==VAL_FLOAT? a->as.f:(double)a->as.i) * (b->kind==VAL_FLOAT? b->as.f:(double)b->as.i));
    if(a->kind==VAL_INT && b->kind==VAL_INT) return v_int(a->as.i * b->as.i);
    return v_void();
}
value_t value_div(const value_t *a, const value_t *b){
    if(both_floaty(a,b)) return v_float((a->kind==VAL_FLOAT? a->as.f:(double)a->as.i) / (b->kind==VAL_FLOAT? b->as.f:(double)b->as.i));
    if(a->kind==VAL_INT && b->kind==VAL_INT) return v_int(b->as.i==0?0:(a->as.i / b->as.i));
    return v_void();
}
value_t value_mod(const value_t *a, const value_t *b){
    if(a->kind==VAL_INT && b->kind==VAL_INT) return v_int(b->as.i==0?0:(a->as.i % b->as.i));
    return v_void();
}
value_t value_eq (const value_t *a, const value_t *b){
    if(a->kind!=b->kind){
        // int==float -> comparar como float
        if(both_floaty(a,b)){
            double aa=(a->kind==VAL_FLOAT?a->as.f:(double)a->as.i);
            double bb=(b->kind==VAL_FLOAT?b->as.f:(double)b->as.i);
            return v_bool(fabs(aa-bb)<1e-12);
        }
        return v_bool(false);
    }
    switch(a->kind){
        case VAL_VOID: return v_bool(true);
        case VAL_BOOL: return v_bool(a->as.b==b->as.b);
        case VAL_INT:  return v_bool(a->as.i==b->as.i);
        case VAL_FLOAT:return v_bool(fabs(a->as.f-b->as.f)<1e-12);
        case VAL_STRING:return v_bool(strcmp(a->as.s? a->as.s:"", b->as.s? b->as.s:"")==0);
        default: return v_bool(false);
    }
}
value_t value_neq(const value_t *a, const value_t *b){ value_t e=value_eq(a,b); e.as.b=!e.as.b; return e; }
value_t value_lt (const value_t *a, const value_t *b){
    if(both_floaty(a,b)){
        double aa=(a->kind==VAL_FLOAT?a->as.f:(double)a->as.i);
        double bb=(b->kind==VAL_FLOAT?b->as.f:(double)b->as.i);
        return v_bool(aa<bb);
    }
    if(a->kind==VAL_INT && b->kind==VAL_INT) return v_bool(a->as.i < b->as.i);
    return v_void();
}
value_t value_lte(const value_t *a, const value_t *b){
    value_t lt=value_lt(a,b); value_t eq=value_eq(a,b);
    value_t r=v_bool( (lt.kind==VAL_BOOL && lt.as.b) || (eq.kind==VAL_BOOL && eq.as.b) ); value_free(&lt); value_free(&eq); return r;
}
value_t value_gt (const value_t *a, const value_t *b){
    value_t le=value_lte(a,b); if(le.kind==VAL_BOOL){ value_t r=v_bool(!le.as.b); value_free(&le); return r; } value_free(&le); return v_void();
}
value_t value_gte(const value_t *a, const value_t *b){
    value_t lt=value_lt(a,b); if(lt.kind==VAL_BOOL){ value_t r=v_bool(!lt.as.b); value_free(&lt); return r; } value_free(&lt); return v_void();
}
value_t value_and(const value_t *a, const value_t *b){
    value_t aa=value_to_bool(a), bb=value_to_bool(b);
    value_t r=v_bool(aa.as.b && bb.as.b); value_free(&aa); value_free(&bb); return r;
}
value_t value_or (const value_t *a, const value_t *b){
    value_t aa=value_to_bool(a), bb=value_to_bool(b);
    value_t r=v_bool(aa.as.b || bb.as.b); value_free(&aa); value_free(&bb); return r;
}
value_t value_not(const value_t *a){
    value_t aa=value_to_bool(a); value_t r=v_bool(!aa.as.b); value_free(&aa); return r;
}
