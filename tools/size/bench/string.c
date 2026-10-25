unsigned my_strlen(const char *s) {
  const char *p = s;
  while (*p)
    p++;
  return p - s;
}

char *my_strcpy(char *d, const char *s) {
  char *r = d;
  while ((*d++ = *s++))
    ;
  return r;
}

int my_strcmp(const char *a, const char *b) {
  while (*a && *a == *b)
    a++, b++;
  return (unsigned char)*a - (unsigned char)*b;
}

void *my_memset(void *d, int c, unsigned n) {
  unsigned char *p = d;
  while (n--)
    *p++ = c;
  return d;
}

char *my_strchr(const char *s, int c) {
  for (; *s; s++)
    if (*s == c)
      return (char *)s;
  return 0;
}

void reverse(char *s, unsigned n) {
  for (unsigned i = 0, j = n - 1; i < j; i++, j--) {
    char t = s[i];
    s[i] = s[j];
    s[j] = t;
  }
}
