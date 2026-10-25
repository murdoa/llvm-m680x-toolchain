// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

char *strchr(const char *s, int c) {
  for (;; ++s) {
    if (*s == (char)c)
      return (char *)s;
    if (!*s)
      return NULL;
  }
}
