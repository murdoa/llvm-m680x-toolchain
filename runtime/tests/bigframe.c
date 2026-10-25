// Functions whose frame is larger than an indexed offset can reach, and
// which return a value in D and X. The epilogue frees such a frame in
// several steps, with the result kept on the stack.

#include <stdint.h>
#include <stdio.h>

__attribute__((noinline)) static void fill(volatile uint8_t *p, unsigned n) {
  for (unsigned i = 0; i < n; ++i)
    p[i] = (uint8_t)(i * 7);
}

#define BIG(name, size)                                                        \
  __attribute__((noinline)) static uint32_t name(uint32_t x) {                 \
    volatile uint8_t buf[size];                                                \
    fill(buf, size);                                                           \
    uint32_t sum = x;                                                          \
    for (unsigned i = 0; i < size; i += 61)                                    \
      sum += buf[i];                                                           \
    return sum;                                                                \
  }

BIG(f252, 252)
BIG(f300, 300)
BIG(f1000, 1000)

static uint32_t expect(uint32_t x, unsigned size) {
  for (unsigned i = 0; i < size; i += 61)
    x += (uint8_t)(i * 7);
  return x;
}

int main(void) {
  int bad = 0;
  if (f252(0x12345678) != expect(0x12345678, 252))
    bad |= 1;
  if (f300(0xdeadbeef) != expect(0xdeadbeef, 300))
    bad |= 2;
  if (f1000(0x00010002) != expect(0x00010002, 1000))
    bad |= 4;
  printf("bigframe %s\n", bad ? "FAIL" : "ok");
  return bad;
}
