// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

size_t strnlen(const char *s, size_t n) {
  size_t i = 0;
  while (i < n && s[i])
    ++i;
  return i;
}
