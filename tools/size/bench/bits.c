unsigned char popcount8(unsigned char v) {
  unsigned char c = 0;
  while (v) {
    c += v & 1;
    v >>= 1;
  }
  return c;
}

unsigned rev16(unsigned v) {
  unsigned r = 0;
  for (int i = 0; i < 16; i++) {
    r = r << 1 | (v & 1);
    v >>= 1;
  }
  return r;
}

unsigned char parity(unsigned v) {
  v ^= v >> 8;
  v ^= v >> 4;
  v ^= v >> 2;
  v ^= v >> 1;
  return (unsigned char)(v & 1);
}

unsigned rotl(unsigned v, unsigned char n) { return v << n | v >> (16 - n); }

void set_bit(unsigned char *map, unsigned bit) { map[bit >> 3] |= (unsigned char)(1 << (bit & 7)); }
int test_bit(const unsigned char *map, unsigned bit) { return map[bit >> 3] >> (bit & 7) & 1; }
