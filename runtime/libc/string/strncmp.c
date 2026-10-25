// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

int strncmp(const char *a, const char *b, size_t n) {
  for (; n; --n, ++a, ++b) {
    if (*a != *b)
      return (unsigned char)*a - (unsigned char)*b;
    if (!*a)
      break;
  }
  return 0;
}
