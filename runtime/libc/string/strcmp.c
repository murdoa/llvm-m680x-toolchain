// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b)
    ++a, ++b;
  return (unsigned char)*a - (unsigned char)*b;
}
