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

void abort(void) { _exit(134); }

extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));

// Called by crt0 before main: C++ constructors and
// __attribute__((constructor)) functions.
void __libc_init_array(void) {
  for (void (**f)(void) = __init_array_start; f != __init_array_end; ++f)
    (*f)();
}

int abs(int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }
long long llabs(long long x) { return x < 0 ? -x : x; }

div_t div(int n, int d) {
  div_t r = {n / d, n % d};
  return r;
}

ldiv_t ldiv(long n, long d) {
  ldiv_t r = {n / d, n % d};
  return r;
}

static unsigned long rand_state = 1;
int rand(void) {
  rand_state = rand_state * 1103515245UL + 12345;
  return (int)((rand_state >> 16) & RAND_MAX);
}
void srand(unsigned seed) { rand_state = seed; }

static int digit_value(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10;
  return 99;
}

unsigned long strtoul(const char *s, char **end, int base) {
  while (*s == ' ' || (*s >= '\t' && *s <= '\r'))
    ++s;
  int neg = 0;
  if (*s == '+' || *s == '-')
    neg = *s++ == '-';
  if ((base == 0 || base == 16) && s[0] == '0' && (s[1] | 32) == 'x') {
    s += 2;
    base = 16;
  } else if (base == 0) {
    base = s[0] == '0' ? 8 : 10;
  }
  unsigned long v = 0;
  int d;
  while ((d = digit_value(*s)) < base) {
    v = v * base + d;
    ++s;
  }
  if (end)
    *end = (char *)s;
  return neg ? -v : v;
}

long strtol(const char *s, char **end, int base) {
  return (long)strtoul(s, end, base);
}

int atoi(const char *s) { return (int)strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }

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
