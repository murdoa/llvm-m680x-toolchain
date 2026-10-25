// Cycle workloads over the size benchmarks (tools/size/bench), minus the
// port-driven control.c. Build with -DWORKLOAD=w_<name>; main returns a
// checksum so that runs on different CPUs can be checked against each
// other.

#include "../size/bench/bits.c"
#include "../size/bench/checksum.c"
#include "../size/bench/convert.c"
#include "../size/bench/long.c"
#include "../size/bench/sort.c"
#include "../size/bench/string.c"
#include "../size/bench/struct.c"

static unsigned char buf[256];
static int ints[64];

static void fill(void) {
  unsigned x = 12345;
  for (unsigned i = 0; i < sizeof buf; i++) {
    x = x * 25173 + 13849;
    buf[i] = (unsigned char)(x >> 8);
  }
  for (unsigned i = 0; i < 64; i++) {
    x = x * 25173 + 13849;
    ints[i] = (int)x;
  }
}

unsigned w_checksum(void) {
  return crc16(buf, sizeof buf) + sum8(buf, sizeof buf) +
         fletcher16(buf, sizeof buf);
}

unsigned w_bits(void) {
  unsigned s = 0;
  unsigned char map[32] = {0};
  for (unsigned i = 0; i < 256; i++) {
    s += popcount8((unsigned char)i) + rev16(i * 257) + parity(i * 31);
    s += rotl(i * 97, (unsigned char)(i & 15));
    set_bit(map, (i * 7) & 255);
    s += test_bit(map, i);
  }
  return s;
}

unsigned w_string(void) {
  char a[64], b[64];
  unsigned s = 0;
  for (unsigned r = 0; r < 8; r++) {
    for (unsigned i = 0; i < 63; i++)
      a[i] = (char)('a' + (buf[i + r] & 15));
    a[63] = 0;
    my_strcpy(b, a);
    reverse(b, my_strlen(b));
    s += my_strlen(a) + (unsigned)my_strcmp(a, b);
    s += (unsigned)(my_strchr(a, 'p') - a);
    my_memset(b, 'x', 32);
    s += (unsigned)my_strcmp(a, b);
  }
  return s;
}

unsigned w_long(void) {
  long la[32];
  unsigned long s = 0;
  for (unsigned i = 0; i < 32; i++)
    la[i] = (long)ints[i] * 37;
  s += (unsigned long)lsum(la, 32);
  for (unsigned i = 0; i < 32; i++) {
    s += umul16x16((unsigned)ints[i], (unsigned)ints[63 - i]);
    s += (unsigned long)scale(la[i], ints[i] & 255, (ints[i + 1] & 127) | 1);
    s += isqrt32((unsigned long)la[i] * (unsigned)ints[i]);
  }
  return (unsigned)(s ^ (s >> 16));
}

unsigned w_sort(void) {
  int a[64];
  unsigned s = 0;
  for (unsigned i = 0; i < 64; i++)
    a[i] = ints[i];
  s += (unsigned)max_index(a, 64);
  isort(a, 64);
  for (unsigned i = 0; i < 64; i++)
    s += (unsigned)bsearch_int(a, 64, ints[i]) + (unsigned)bsearch_int(a, 64, i);
  return s;
}

unsigned w_struct(void) {
  static struct node nodes[32];
  static struct ring ring;
  unsigned s = 0;
  for (unsigned i = 0; i < 32; i++) {
    nodes[i].next = i < 31 ? &nodes[i + 1] : 0;
    nodes[i].key = ints[i];
    nodes[i].tag = (char)(buf[i] & 3);
  }
  struct node *l = nodes;
  for (unsigned r = 0; r < 4; r++) {
    for (unsigned i = 0; i < 32; i += 3)
      s += find(l, ints[i]) ? 1 : 0;
    l = list_reverse(l);
    s += count_tag(l, (char)r);
  }
  for (unsigned i = 0; i < 100; i++) {
    s += (unsigned)ring_put(&ring, buf[i]);
    if (i & 1)
      s += (unsigned)ring_get(&ring);
  }
  return s;
}

unsigned w_convert(void) {
  char t[8];
  unsigned s = 0;
  unsigned char bcd[4] = {0x12, 0x34, 0x56, 0x78};
  for (unsigned i = 0; i < 64; i++) {
    utoa10((unsigned)ints[i], t);
    s += (unsigned)my_atoi(t);
    tohex((unsigned)ints[i], t);
    s += (unsigned char)t[0] + (unsigned char)t[3];
    s += bcd2bin(bin2bcd((unsigned char)(i + 20)));
    s += bcd_add(bcd, bcd, 4);
  }
  return s;
}

int main(void) {
  fill();
  return (int)(WORKLOAD() % 127);
}
