#include <stdlib.h>

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
