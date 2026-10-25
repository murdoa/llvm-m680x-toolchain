// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

char *strstr(const char *h, const char *n) {
  size_t len = strlen(n);
  for (; *h; ++h)
    if (!strncmp(h, n, len))
      return (char *)h;
  return len ? NULL : (char *)h;
}
