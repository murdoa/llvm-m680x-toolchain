long lsum(const long *a, unsigned n) {
  long s = 0;
  while (n--)
    s += *a++;
  return s;
}

unsigned long umul16x16(unsigned a, unsigned b) { return (unsigned long)a * b; }

long scale(long v, int num, int den) { return v * num / den; }

unsigned long isqrt32(unsigned long v) {
  unsigned long r = 0, bit = 1UL << 30;
  while (bit > v)
    bit >>= 2;
  while (bit) {
    if (v >= r + bit) {
      v -= r + bit;
      r = (r >> 1) + bit;
    } else
      r >>= 1;
    bit >>= 2;
  }
  return r;
}
