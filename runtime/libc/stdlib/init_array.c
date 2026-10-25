#include <stdlib.h>

extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));

// Called by crt0 before main: C++ constructors and
// __attribute__((constructor)) functions.
void __libc_init_array(void) {
  for (void (**f)(void) = __init_array_start; f != __init_array_end; ++f)
    (*f)();
}
