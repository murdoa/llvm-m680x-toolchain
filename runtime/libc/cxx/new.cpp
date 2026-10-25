// operator new and delete on malloc and free. There are no exceptions, so
// running out of memory aborts.

#include <stdlib.h>

typedef __SIZE_TYPE__ size_t;

static void *allocate(size_t n) {
  void *p = malloc(n ? n : 1);
  if (!p)
    abort();
  return p;
}

void *operator new(size_t n) { return allocate(n); }
void *operator new[](size_t n) { return allocate(n); }
void operator delete(void *p) noexcept { free(p); }
void operator delete[](void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }
void operator delete[](void *p, size_t) noexcept { free(p); }
