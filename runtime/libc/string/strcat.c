// Built with -fno-builtin, so the loops here are not turned back into
// calls to the function itself.

#include <string.h>

char *strcat(char *restrict d, const char *restrict s) {
  strcpy(d + strlen(d), s);
  return d;
}
