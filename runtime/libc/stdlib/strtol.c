#include <stdlib.h>

long strtol(const char *s, char **end, int base) {
  return (long)strtoul(s, end, base);
}
