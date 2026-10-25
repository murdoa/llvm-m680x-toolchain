#include <stdlib.h>

#define MAX_ATEXIT 8
static void (*atexit_fns[MAX_ATEXIT])(void);
static int atexit_count;

int atexit(void (*fn)(void)) {
  if (atexit_count == MAX_ATEXIT)
    return -1;
  atexit_fns[atexit_count++] = fn;
  return 0;
}

extern void (*__fini_array_start[])(void) __attribute__((weak));
extern void (*__fini_array_end[])(void) __attribute__((weak));

void exit(int status) {
  while (atexit_count)
    atexit_fns[--atexit_count]();
  for (void (**f)(void) = __fini_array_end; f != __fini_array_start;)
    (*--f)();
  _exit(status);
}
