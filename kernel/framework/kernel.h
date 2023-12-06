#include <am.h>

#define MODULE(mod) \
  typedef struct mod_##mod##_t mod_##mod##_t; \
  extern mod_##mod##_t *mod; \
  struct mod_##mod##_t

#define MODULE_DEF(mod) \
  extern mod_##mod##_t __##mod##_obj; \
  mod_##mod##_t *mod = &__##mod##_obj; \
  mod_##mod##_t __##mod##_obj
// there is a useful trick here! we first use extern to declare mod and then to define it; think how it works...
// .eg:
/*

  extern int num;
  int mul(int a){
    return a*num;
    // num is defined after this line but it works! since we use extern to declare before
  }
  int num = 10;

*/




MODULE(os) {
  void (*init)();
  void (*run)();
};

MODULE(pmm) {
  void  (*init)();
  void *(*alloc)(size_t size);
  void  (*free)(void *ptr);
};
