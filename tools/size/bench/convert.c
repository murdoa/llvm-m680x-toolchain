char *utoa10(unsigned v, char *buf) {
  char *p = buf + 6;
  *--p = 0;
  do {
    *--p = '0' + v % 10;
    v /= 10;
  } while (v);
  return p;
}

int my_atoi(const char *s) {
  int neg = 0, v = 0;
  while (*s == ' ')
    s++;
  if (*s == '-')
    neg = 1, s++;
  while (*s >= '0' && *s <= '9')
    v = v * 10 + (*s++ - '0');
  return neg ? -v : v;
}

void tohex(unsigned v, char *out) {
  static const char digits[] = "0123456789ABCDEF";
  for (int i = 3; i >= 0; i--) {
    out[i] = digits[v & 15];
    v >>= 4;
  }
  out[4] = 0;
}

unsigned char bin2bcd(unsigned char v) { return (v / 10) << 4 | v % 10; }
unsigned char bcd2bin(unsigned char v) { return (v >> 4) * 10 + (v & 15); }

/* Packed BCD add of n bytes, most significant first. */
unsigned char bcd_add(unsigned char *a, const unsigned char *b, unsigned char n) {
  unsigned char carry = 0;
  while (n--) {
    unsigned char lo = (a[n] & 15) + (b[n] & 15) + carry;
    unsigned char hi = (a[n] >> 4) + (b[n] >> 4);
    if (lo > 9)
      lo -= 10, hi++;
    carry = hi > 9;
    if (carry)
      hi -= 10;
    a[n] = hi << 4 | lo;
  }
  return carry;
}
