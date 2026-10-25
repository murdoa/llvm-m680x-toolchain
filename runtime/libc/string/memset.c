// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

void *memset(void *d, int c, size_t n) {
  unsigned char *dp = d;
  while (n--)
    *dp++ = (unsigned char)c;
  return d;
}
