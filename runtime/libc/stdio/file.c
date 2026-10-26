// The standard streams and character output, apart from printf so that a
// program using only these does not bring in the formatter.

#include <stdio.h>

int __board_putc(int c);
int __board_getc(void);

struct __file {
  int unused;
};
static struct __file files[3];
FILE *const stdin = &files[0];
FILE *const stdout = &files[1];
FILE *const stderr = &files[2];

int putchar(int c) { return __board_putc(c); }
int getchar(void) { return __board_getc(); }
int fputc(int c, FILE *f) { return __board_putc(c); }

int fputs(const char *s, FILE *f) {
  while (*s)
    __board_putc(*s++);
  return 0;
}

int puts(const char *s) {
  fputs(s, stdout);
  __board_putc('\n');
  return 0;
}

// The compiler turns fputs and printf of a constant string into fwrite.
size_t fwrite(const void *p, size_t size, size_t n, FILE *f) {
  const unsigned char *c = p;
  for (size_t i = 0; i < size * n; ++i)
    __board_putc(c[i]);
  return n;
}
