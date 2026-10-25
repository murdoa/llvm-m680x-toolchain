// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

char *strncpy(char *restrict d, const char *restrict s, size_t n) {
  char *r = d;
  for (; n && *s; --n)
    *d++ = *s++;
  for (; n; --n)
    *d++ = 0;
  return r;
}
