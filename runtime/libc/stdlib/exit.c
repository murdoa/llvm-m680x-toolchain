#include <stdlib.h>

// Handlers from atexit and from __cxa_atexit, which C++ uses to destroy
// static objects. They run in reverse order of registration.
#define MAX_ATEXIT 16
static struct {
  void (*fn)(void);
  void (*cxa_fn)(void *);
  void *arg;
} handlers[MAX_ATEXIT];
static int count;

int atexit(void (*fn)(void)) {
  if (count == MAX_ATEXIT)
    return -1;
  handlers[count].fn = fn;
  handlers[count].cxa_fn = 0;
  ++count;
  return 0;
}

int __cxa_atexit(void (*fn)(void *), void *arg, void *dso) {
  (void)dso;
  if (count == MAX_ATEXIT)
    return -1;
  handlers[count].fn = 0;
  handlers[count].cxa_fn = fn;
  handlers[count].arg = arg;
  ++count;
  return 0;
}

// Identifies this program to __cxa_atexit. There are no shared objects.
void *__dso_handle = &__dso_handle;

extern void (*__fini_array_start[])(void) __attribute__((weak));
extern void (*__fini_array_end[])(void) __attribute__((weak));

void exit(int status) {
  while (count) {
    --count;
    if (handlers[count].fn)
      handlers[count].fn();
    else
      handlers[count].cxa_fn(handlers[count].arg);
  }
  for (void (**f)(void) = __fini_array_end; f != __fini_array_start;)
    (*--f)();
  _exit(status);
}
