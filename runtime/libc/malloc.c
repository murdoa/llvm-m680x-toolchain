// A first-fit allocator over the space between the end of .bss and the
// stack. Each block has a 2-byte header: its size, with bit 0 set when free.

#include <stdlib.h>
#include <string.h>

extern char __heap_start[] __attribute__((weak));

#define STACK_RESERVE 512

static char *heap_end;

static size_t *first_block(void) { return (size_t *)__heap_start; }

void *malloc(size_t n) {
  if (!n)
    return NULL;
  n = (n + 1) & ~(size_t)1; // keep sizes even; bit 0 is the free flag
  if (!heap_end)
    heap_end = __heap_start;
  for (size_t *b = first_block(); (char *)b < heap_end;
       b = (size_t *)((char *)b + 2 + (*b & ~(size_t)1))) {
    size_t size = *b & ~(size_t)1;
    if ((*b & 1) && size >= n) {
      if (size >= n + 4) { // split
        size_t *rest = (size_t *)((char *)b + 2 + n);
        *rest = (size - n - 2) | 1;
        size = n;
      }
      *b = size;
      return b + 1;
    }
  }
  char sp_probe;
  if ((size_t)(&sp_probe - heap_end) < n + 2 + STACK_RESERVE)
    return NULL;
  size_t *b = (size_t *)heap_end;
  *b = n;
  heap_end += 2 + n;
  return b + 1;
}

void free(void *p) {
  if (p)
    ((size_t *)p)[-1] |= 1;
}

void *calloc(size_t n, size_t size) {
  size_t total = n * size;
  if (size && total / size != n)
    return NULL;
  void *p = malloc(total);
  if (p)
    memset(p, 0, total);
  return p;
}

void *realloc(void *p, size_t n) {
  if (!p)
    return malloc(n);
  size_t old = ((size_t *)p)[-1] & ~(size_t)1;
  if (n <= old)
    return p;
  void *q = malloc(n);
  if (q) {
    memcpy(q, p, old);
    free(p);
  }
  return q;
}
