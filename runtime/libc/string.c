// String and memory functions. Written plainly: the compiler must not turn
// these loops back into calls to themselves, so this file is built with
// -fno-builtin.

#include <string.h>

void *memcpy(void *restrict d, const void *restrict s, size_t n) {
  unsigned char *dp = d;
  const unsigned char *sp = s;
  while (n--)
    *dp++ = *sp++;
  return d;
}

void *memmove(void *d, const void *s, size_t n) {
  unsigned char *dp = d;
  const unsigned char *sp = s;
  if (dp < sp) {
    while (n--)
      *dp++ = *sp++;
  } else {
    dp += n;
    sp += n;
    while (n--)
      *--dp = *--sp;
  }
  return d;
}

void *memset(void *d, int c, size_t n) {
  unsigned char *dp = d;
  while (n--)
    *dp++ = (unsigned char)c;
  return d;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *ap = a, *bp = b;
  for (; n; --n, ++ap, ++bp)
    if (*ap != *bp)
      return *ap < *bp ? -1 : 1;
  return 0;
}

void *memchr(const void *s, int c, size_t n) {
  const unsigned char *p = s;
  for (; n; --n, ++p)
    if (*p == (unsigned char)c)
      return (void *)p;
  return NULL;
}

size_t strlen(const char *s) {
  const char *p = s;
  while (*p)
    ++p;
  return (size_t)(p - s);
}

size_t strnlen(const char *s, size_t n) {
  size_t i = 0;
  while (i < n && s[i])
    ++i;
  return i;
}

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b)
    ++a, ++b;
  return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
  for (; n; --n, ++a, ++b) {
    if (*a != *b)
      return (unsigned char)*a - (unsigned char)*b;
    if (!*a)
      break;
  }
  return 0;
}

char *strcpy(char *restrict d, const char *restrict s) {
  char *r = d;
  while ((*d++ = *s++))
    ;
  return r;
}

char *strncpy(char *restrict d, const char *restrict s, size_t n) {
  char *r = d;
  for (; n && *s; --n)
    *d++ = *s++;
  for (; n; --n)
    *d++ = 0;
  return r;
}

char *strcat(char *restrict d, const char *restrict s) {
  strcpy(d + strlen(d), s);
  return d;
}

char *strncat(char *restrict d, const char *restrict s, size_t n) {
  char *p = d + strlen(d);
  for (; n && *s; --n)
    *p++ = *s++;
  *p = 0;
  return d;
}

char *strchr(const char *s, int c) {
  for (;; ++s) {
    if (*s == (char)c)
      return (char *)s;
    if (!*s)
      return NULL;
  }
}

char *strrchr(const char *s, int c) {
  const char *r = NULL;
  for (;; ++s) {
    if (*s == (char)c)
      r = s;
    if (!*s)
      return (char *)r;
  }
}

char *strstr(const char *h, const char *n) {
  size_t len = strlen(n);
  for (; *h; ++h)
    if (!strncmp(h, n, len))
      return (char *)h;
  return len ? NULL : (char *)h;
}
