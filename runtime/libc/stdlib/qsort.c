#include <stdlib.h>

void qsort(void *base, size_t n, size_t size,
           int (*cmp)(const void *, const void *)) {
  // Insertion sort: small and fine for the sizes a 6809 sorts.
  unsigned char *b = base;
  for (size_t i = 1; i < n; ++i)
    for (size_t j = i; j > 0 && cmp(b + (j - 1) * size, b + j * size) > 0;
         --j)
      for (size_t k = 0; k < size; ++k) {
        unsigned char t = b[(j - 1) * size + k];
        b[(j - 1) * size + k] = b[j * size + k];
        b[j * size + k] = t;
      }
}
