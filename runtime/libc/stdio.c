// A small printf family: %d %i %u %x %X %o %c %s %p %%, the h/l/ll/z
// length modifiers, and flags "-0+ #" with width and precision. No floating
// point.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int __m6809_putc(int c);
int __m6809_getc(void);

struct __file {
  int unused;
};
static struct __file files[3];
FILE *const stdin = &files[0];
FILE *const stdout = &files[1];
FILE *const stderr = &files[2];

int putchar(int c) { return __m6809_putc(c); }
int getchar(void) { return __m6809_getc(); }
int fputc(int c, FILE *f) { return __m6809_putc(c); }

int fputs(const char *s, FILE *f) {
  while (*s)
    __m6809_putc(*s++);
  return 0;
}

int puts(const char *s) {
  fputs(s, stdout);
  __m6809_putc('\n');
  return 0;
}

// Output goes to the console, or to a buffer when buf is set.
struct out {
  char *buf;
  size_t size, len;
};

static void emit(struct out *o, char c) {
  if (o->buf) {
    if (o->len + 1 < o->size)
      o->buf[o->len] = c;
  } else {
    __m6809_putc(c);
  }
  ++o->len;
}

static void pad(struct out *o, char c, int n) {
  while (n-- > 0)
    emit(o, c);
}

static int format(struct out *o, const char *fmt, va_list ap) {
  for (; *fmt; ++fmt) {
    if (*fmt != '%') {
      emit(o, *fmt);
      continue;
    }
    int left = 0, zero = 0, plus = 0, space = 0, alt = 0;
    for (;; ++fmt) {
      char f = fmt[1];
      if (f == '-')
        left = 1;
      else if (f == '0')
        zero = 1;
      else if (f == '+')
        plus = 1;
      else if (f == ' ')
        space = 1;
      else if (f == '#')
        alt = 1;
      else
        break;
    }
    ++fmt;
    int width = 0, prec = -1;
    if (*fmt == '*') {
      width = va_arg(ap, int);
      if (width < 0)
        left = 1, width = -width;
      ++fmt;
    } else {
      while (*fmt >= '0' && *fmt <= '9')
        width = width * 10 + (*fmt++ - '0');
    }
    if (*fmt == '.') {
      ++fmt;
      prec = 0;
      if (*fmt == '*') {
        prec = va_arg(ap, int);
        ++fmt;
      } else {
        while (*fmt >= '0' && *fmt <= '9')
          prec = prec * 10 + (*fmt++ - '0');
      }
    }
    int size = 0; // 0 int, 1 long, 2 long long, -1 short, -2 char
    for (;; ++fmt) {
      if (*fmt == 'l')
        ++size;
      else if (*fmt == 'h')
        --size;
      else if (*fmt == 'z' || *fmt == 't')
        size = 0;
      else if (*fmt == 'j')
        size = 2;
      else
        break;
    }
    char conv = *fmt;
    if (!conv)
      break;

    if (conv == 'c') {
      if (!left)
        pad(o, ' ', width - 1);
      emit(o, (char)va_arg(ap, int));
      if (left)
        pad(o, ' ', width - 1);
      continue;
    }
    if (conv == 's') {
      const char *s = va_arg(ap, const char *);
      if (!s)
        s = "(null)";
      int len = (int)strlen(s);
      if (prec >= 0 && len > prec)
        len = prec;
      if (!left)
        pad(o, ' ', width - len);
      for (int i = 0; i < len; ++i)
        emit(o, s[i]);
      if (left)
        pad(o, ' ', width - len);
      continue;
    }
    if (conv == '%') {
      emit(o, '%');
      continue;
    }

    unsigned base = 10;
    int is_signed = 0, upper = 0;
    switch (conv) {
    case 'd':
    case 'i':
      is_signed = 1;
      break;
    case 'u':
      break;
    case 'x':
      base = 16;
      break;
    case 'X':
      base = 16;
      upper = 1;
      break;
    case 'o':
      base = 8;
      break;
    case 'p':
      base = 16;
      alt = 1;
      size = 0;
      break;
    default:
      emit(o, '%');
      emit(o, conv);
      continue;
    }

    unsigned long long v;
    int neg = 0;
    if (is_signed) {
      long long s = size >= 2   ? va_arg(ap, long long)
                    : size == 1 ? va_arg(ap, long)
                                : va_arg(ap, int);
      if (size == -1)
        s = (short)s;
      else if (size <= -2)
        s = (signed char)s;
      if (s < 0) {
        neg = 1;
        v = -(unsigned long long)s;
      } else {
        v = (unsigned long long)s;
      }
    } else {
      v = size >= 2   ? va_arg(ap, unsigned long long)
          : size == 1 ? va_arg(ap, unsigned long)
                      : va_arg(ap, unsigned);
      if (size == -1)
        v = (unsigned short)v;
      else if (size <= -2)
        v = (unsigned char)v;
    }

    char digits[24];
    int n = 0;
    const char *set = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    while (v) {
      digits[n++] = set[v % base];
      v /= base;
    }
    if (prec < 0 && !n)
      digits[n++] = '0';
    int zeros = prec > n ? prec - n : 0;
    const char *prefix = "";
    if (neg)
      prefix = "-";
    else if (plus && is_signed)
      prefix = "+";
    else if (space && is_signed)
      prefix = " ";
    else if (alt && base == 16 && (n || conv == 'p'))
      prefix = upper ? "0X" : "0x";
    else if (alt && base == 8 && !zeros && (!n || digits[n - 1] != '0'))
      prefix = "0";
    int plen = (int)strlen(prefix);
    int total = plen + zeros + n;
    if (zero && !left && prec < 0) {
      zeros += width - total;
      total = width;
    }
    if (!left)
      pad(o, ' ', width - total);
    for (int i = 0; i < plen; ++i)
      emit(o, prefix[i]);
    pad(o, '0', zeros);
    while (n)
      emit(o, digits[--n]);
    if (left)
      pad(o, ' ', width - total);
  }
  if (o->buf && o->size)
    o->buf[o->len < o->size ? o->len : o->size - 1] = 0;
  return (int)o->len;
}

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
  struct out o = {buf, n, 0};
  if (!buf) {
    // Count only.
    static char dummy;
    o.buf = &dummy;
    o.size = 0;
  }
  return format(&o, fmt, ap);
}

int vsprintf(char *buf, const char *fmt, va_list ap) {
  return vsnprintf(buf, (size_t)-1, fmt, ap);
}

int vprintf(const char *fmt, va_list ap) {
  struct out o = {NULL, 0, 0};
  return format(&o, fmt, ap);
}

int vfprintf(FILE *f, const char *fmt, va_list ap) { return vprintf(fmt, ap); }

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vprintf(fmt, ap);
  va_end(ap);
  return r;
}

int fprintf(FILE *f, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vprintf(fmt, ap);
  va_end(ap);
  return r;
}

int sprintf(char *buf, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsprintf(buf, fmt, ap);
  va_end(ap);
  return r;
}

int snprintf(char *buf, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(buf, n, fmt, ap);
  va_end(ap);
  return r;
}
