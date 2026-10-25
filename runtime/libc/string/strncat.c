// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

char *strncat(char *restrict d, const char *restrict s, size_t n) {
  char *p = d + strlen(d);
  for (; n && *s; --n)
    *p++ = *s++;
  *p = 0;
  return d;
}
