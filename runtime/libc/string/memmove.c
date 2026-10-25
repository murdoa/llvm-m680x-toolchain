// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

void *memmove(void *d, const void *s, size_t n) {
  unsigned char *dp = d;
  const unsigned char *sp = s;
  if (dp < sp) {
    while (n--)
      *dp++ = *sp++;
  } else {
    dp += n;
    sp += n;
    while (n--)
      *--dp = *--sp;
  }
  return d;
}
