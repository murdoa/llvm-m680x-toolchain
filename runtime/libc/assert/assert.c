#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void __assert_fail(const char *expr, const char *file, int line,
                   const char *func) {
  fprintf(stderr, "%s:%d: %s: assertion `%s' failed\n", file, line, func,
          expr);
  abort();
}
