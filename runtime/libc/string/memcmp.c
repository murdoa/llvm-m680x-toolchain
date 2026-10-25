// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *ap = a, *bp = b;
  for (; n; --n, ++ap, ++bp)
    if (*ap != *bp)
      return *ap < *bp ? -1 : 1;
  return 0;
}
