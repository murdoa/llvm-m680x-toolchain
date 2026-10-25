// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

void *memcpy(void *restrict d, const void *restrict s, size_t n) {
  unsigned char *dp = d;
  const unsigned char *sp = s;
  while (n--)
    *dp++ = *sp++;
  return d;
}
