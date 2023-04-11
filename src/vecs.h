#include <stdint.h>

#define DEF_VECTOR_STRU(name, type) \
struct name ##v {       \
  type *__restrict v;   \
  uint32_t l;           \
  uint32_t s;           \
};

#define VECTOR_PUSH(name, vect, type)                   \
void name ##vp(vect v, type val) {                      \
  if (v->l == v->s) {                                 \
    v->s *= 2;                                        \
    v->v = realloc(v->v, sizeof(v->v[0]) * v->s);     \
  }                                                   \
  v->v[v->l] = val;                                   \
  ++v->l;                                             \
}                                                     

#define VECTOR_INIT(name, vect)                      \
void name ##vi(vect v) {                             \
  v->l = 0;                                        \
  v->s = 4;                                        \
  v->v = malloc(sizeof(v->v[0]) * v->s);           \
}                                             

#define VECTOR_TRIM(name, vect)                      \
void name ##vt(vect v) {                          \
  if (v->s != v->l) {                              \
    v->s = v->l;                                   \
    v->v = realloc(v->v, sizeof(v->v[0]) * v->s);  \
  }                                                \
}

#define VECTOR_SUITE(name, type) \
VECTOR_PUSH(name, struct name ##v *__restrict, type) \
VECTOR_INIT(name, struct name ##v *__restrict) \
VECTOR_TRIM(name, struct name ##v *__restrict)

#define DEF_VECTOR_PUSH(name, vect, type) \
void name ##vp(vect v, type val);

#define DEF_VECTOR_INIT(name, vect) \
void name ##vi(vect v);

#define DEF_VECTOR_TRIM(name, vect) \
void name ##vt(vect v);

#define DEF_VECTOR_SUITE(name, type) \
DEF_VECTOR_STRU(name, type) \
DEF_VECTOR_PUSH(name, struct name ##v *__restrict, type) \
DEF_VECTOR_INIT(name, struct name ##v *__restrict) \
DEF_VECTOR_TRIM(name, struct name ##v *__restrict)
