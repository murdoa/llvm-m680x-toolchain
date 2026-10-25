// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

char *strcpy(char *restrict d, const char *restrict s) {
  char *r = d;
  while ((*d++ = *s++))
    ;
  return r;
}
