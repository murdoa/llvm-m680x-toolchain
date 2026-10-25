// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

size_t strlen(const char *s) {
  const char *p = s;
  while (*p)
    ++p;
  return (size_t)(p - s);
}
