// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

char *strrchr(const char *s, int c) {
  const char *r = NULL;
  for (;; ++s) {
    if (*s == (char)c)
      r = s;
    if (!*s)
      return (char *)r;
  }
}
