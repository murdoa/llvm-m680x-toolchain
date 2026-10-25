// A small printf family: %d %i %u %x %X %o %c %s %p %f %e %g %%, the
// h/l/ll/z/L length modifiers, and flags "-0+ #" with width and precision.
// Floating point is printed from 17 significant digits, so the digits past
// those are zeros.

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

//===----------------------------------------------------------------------===//
// Floating point
//===----------------------------------------------------------------------===//

#define SIG 17

// The power of ten of the first digit of x > 0, give or take one.
static int decimal_exponent(double x) {
  static const double pow10[] = {1e256, 1e128, 1e64, 1e32, 1e16,
                                 1e8,   1e4,   1e2,  1e1};
  static const short exp10[] = {256, 128, 64, 32, 16, 8, 4, 2, 1};
  int e = 0;
  if (x >= 10) {
    for (int i = 0; i < 9; ++i)
      if (x >= pow10[i]) {
        x /= pow10[i];
        e += exp10[i];
      }
  } else if (x < 1) {
    for (int i = 0; i < 9; ++i)
      if (x * pow10[i] < 10) {
        x *= pow10[i];
        e -= exp10[i];
      }
  }
  return e;
}

// 10^k, exactly for k <= 22.
static double pow10_exact(int k) {
  double p = 1;
  while (k--)
    p *= 10;
  return p;
}

// The first SIG decimal digits of x > 0, and the power of ten of the first.
// x is scaled to a SIG-digit integer by one multiply or divide by an exact
// power of ten when it can be (from 1e-6 to 1e38), so the digits are those
// of x correctly rounded to SIG digits.
static int decimal_digits(double x, char *d) {
  const unsigned long long lo = 10000000000000000ULL; // 10^(SIG-1)
  int e = decimal_exponent(x);
  for (int tries = 0;; ++tries) {
    int k = SIG - 1 - e;
    double s = x;
    while (k > 22) {
      s *= 1e22;
      k -= 22;
    }
    while (k < -22) {
      s /= 1e22;
      k += 22;
    }
    s = k >= 0 ? s * pow10_exact(k) : s / pow10_exact(-k);
    unsigned long long n = (unsigned long long)(s + 0.5);
    if (tries < 2 && n >= 10 * lo) {
      ++e;
      continue;
    }
    if (tries < 2 && n < lo) {
      --e;
      continue;
    }
    if (n >= 10 * lo) {
      n = (n + 5) / 10;
      ++e;
    }
    for (int i = SIG - 1; i >= 0; --i) {
      d[i] = (char)(n % 10);
      n /= 10;
    }
    return e;
  }
}

// Rounds to n significant digits, ties to even. Returns the new exponent,
// which a carry out of the first digit raises.
static int round_digits(char *d, int n, int e) {
  if (n >= SIG)
    return e;
  if (n < 0) {
    memset(d, 0, SIG);
    return e;
  }
  int up = d[n] > 5;
  if (d[n] == 5) {
    up = n > 0 && (d[n - 1] & 1);
    for (int i = n + 1; i < SIG; ++i)
      if (d[i])
        up = 1;
  }
  memset(d + n, 0, SIG - n);
  if (up) {
    int k = n - 1;
    while (k >= 0 && d[k] == 9)
      d[k--] = 0;
    if (k >= 0) {
      ++d[k];
    } else {
      memmove(d + 1, d, SIG - 1);
      d[0] = 1;
      ++e;
    }
  }
  return e;
}

// The digit for 10^p.
static char digit_at(const char *d, int e, int p) {
  int i = e - p;
  return (char)('0' + (i >= 0 && i < SIG ? d[i] : 0));
}

// Writes the digits of a %f, %e or %g conversion, without sign or padding.
static void float_body(struct out *o, const char *d, int e, char conv,
                       int prec, int alt) {
  int upper = conv < 'a';
  conv |= 32;
  int trim = 0;
  if (conv == 'g') {
    // %e style for exponents below -4 or from the precision up.
    if (prec > e && e >= -4) {
      conv = 'f';
      prec = prec - 1 - e;
    } else {
      conv = 'e';
      prec = prec - 1;
    }
    trim = !alt;
  }
  // Without trailing zeros (for %g), fewer fraction digits are printed.
  int last = conv == 'f' ? -prec : e - prec;
  if (trim)
    while (last < (conv == 'f' ? 0 : e) && digit_at(d, e, last) == '0')
      ++last;
  int first = conv == 'f' ? (e > 0 ? e : 0) : e;
  emit(o, digit_at(d, e, first));
  for (int p = first - 1; p >= last; --p) {
    if (p == (conv == 'f' ? -1 : e - 1))
      emit(o, '.');
    emit(o, digit_at(d, e, p));
  }
  if (alt && last == (conv == 'f' ? 0 : e))
    emit(o, '.');
  if (conv == 'e') {
    emit(o, upper ? 'E' : 'e');
    emit(o, e < 0 ? '-' : '+');
    int x = e < 0 ? -e : e;
    if (x >= 100)
      emit(o, (char)('0' + x / 100));
    emit(o, (char)('0' + x / 10 % 10));
    emit(o, (char)('0' + x % 10));
  }
}

static void format_float(struct out *o, double x, char conv, int prec,
                         int width, int left, int zero, int plus, int space,
                         int alt) {
  union {
    double d;
    unsigned long long u;
  } bits = {x};
  int neg = (int)(bits.u >> 63);
  if (neg)
    x = -x;
  const char *sign = neg ? "-" : plus ? "+" : space ? " " : "";
  int slen = *sign ? 1 : 0;

  if (x != x || x > 1.7976931348623157e308) {
    const char *s = x != x ? (conv < 'a' ? "NAN" : "nan")
                           : (conv < 'a' ? "INF" : "inf");
    if (!left)
      pad(o, ' ', width - 3 - slen);
    if (slen)
      emit(o, *sign);
    for (int i = 0; i < 3; ++i)
      emit(o, s[i]);
    if (left)
      pad(o, ' ', width - 3 - slen);
    return;
  }

  if (prec < 0)
    prec = 6;
  char d[SIG];
  int e = 0;
  if (x == 0)
    memset(d, 0, SIG);
  else
    e = decimal_digits(x, d);
  switch (conv | 32) {
  case 'f':
    e = round_digits(d, e + 1 + prec, e);
    break;
  case 'e':
    e = round_digits(d, prec + 1, e);
    break;
  default:
    if (prec == 0)
      prec = 1;
    e = round_digits(d, prec, e);
    break;
  }
  if (x == 0 || (d[0] == 0 && (conv | 32) == 'f'))
    e = 0;

  struct out count = {(char *)"", 0, 0}; // counts without writing
  float_body(&count, d, e, conv, prec, alt);
  int total = slen + (int)count.len;
  if (!left && !zero)
    pad(o, ' ', width - total);
  if (slen)
    emit(o, *sign);
  if (!left && zero)
    pad(o, '0', width - total);
  float_body(o, d, e, conv, prec, alt);
  if (left)
    pad(o, ' ', width - total);
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
      else if (*fmt == 'L')
        size = 0; // long double is double
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
    if ((conv | 32) == 'f' || (conv | 32) == 'e' || (conv | 32) == 'g') {
      format_float(o, va_arg(ap, double), conv, prec, width, left, zero, plus,
                   space, alt);
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
