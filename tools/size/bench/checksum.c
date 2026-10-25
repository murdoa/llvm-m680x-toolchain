unsigned crc16(const unsigned char *p, unsigned n) {
  unsigned crc = 0xFFFF;
  while (n--) {
    crc ^= *p++;
    for (int i = 0; i < 8; i++)
      crc = crc & 1 ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

unsigned char sum8(const unsigned char *p, unsigned n) {
  unsigned char s = 0;
  while (n--)
    s += *p++;
  return s;
}

unsigned fletcher16(const unsigned char *p, unsigned n) {
  unsigned a = 0, b = 0;
  for (unsigned i = 0; i < n; i++) {
    a = (a + p[i]) % 255;
    b = (b + a) % 255;
  }
  return b << 8 | a;
}
