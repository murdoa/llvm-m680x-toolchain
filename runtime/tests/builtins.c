// Checks the arithmetic helpers against slow reference versions that use
// only operations the compiler does inline (16-bit add, subtract, compare,
// and one-bit shifts).

#include <stdint.h>
#include <stdio.h>

static uint16_t rng = 0xACE1;
static uint16_t rand16(void) {
  rng ^= rng << 7;
  rng ^= rng >> 9;
  rng ^= rng << 8;
  return rng;
}
static uint32_t rand32(void) {
  uint32_t hi = rand16();
  return (hi << 16) | rand16();
}

// The reference functions must not be turned into the helpers they check.
#define NOINLINE __attribute__((noinline))

NOINLINE static uint16_t ref_mul16(uint16_t a, uint16_t b) {
  uint16_t r = 0;
  for (int i = 0; i < 16; ++i) {
    if (b & 1)
      r += a;
    a <<= 1;
    b >>= 1;
  }
  return r;
}

NOINLINE static uint32_t ref_mul32(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (int i = 0; i < 32; ++i) {
    if (b & 1)
      r += a;
    a <<= 1;
    b >>= 1;
  }
  return r;
}

NOINLINE static uint16_t ref_udiv16(uint16_t n, uint16_t d, uint16_t *rem) {
  uint16_t q = 0, r = 0;
  for (int i = 0; i < 16; ++i) {
    r = (r << 1) | (n >> 15);
    n <<= 1;
    q <<= 1;
    if (r >= d) {
      r -= d;
      q |= 1;
    }
  }
  *rem = r;
  return q;
}

NOINLINE static uint32_t ref_udiv32(uint32_t n, uint32_t d, uint32_t *rem) {
  uint32_t q = 0, r = 0;
  for (int i = 0; i < 32; ++i) {
    // r can reach 2d - 1, which overflows when d is above 2^31.
    int carry = (r >> 31) & 1;
    r = (r << 1) | (n >> 31);
    n <<= 1;
    q <<= 1;
    if (carry || r >= d) {
      r -= d;
      q |= 1;
    }
  }
  *rem = r;
  return q;
}

NOINLINE static uint32_t ref_shl32(uint32_t v, int n) {
  while (n--)
    v <<= 1;
  return v;
}
NOINLINE static uint32_t ref_lshr32(uint32_t v, int n) {
  while (n--)
    v >>= 1;
  return v;
}
NOINLINE static int32_t ref_ashr32(int32_t v, int n) {
  while (n--)
    v >>= 1;
  return v;
}

static int failures;
static void check(const char *what, uint32_t got, uint32_t want, uint32_t a,
                  uint32_t b) {
  if (got == want)
    return;
  if (failures++ < 10)
    printf("%s(%lx, %lx) = %lx, want %lx\n", what, (unsigned long)a,
           (unsigned long)b, (unsigned long)got, (unsigned long)want);
}

// Called through volatile function pointers' worth of opacity: the inputs
// come from the RNG, so nothing folds.
int main(void) {
  for (int i = 0; i < 300; ++i) {
    uint16_t a = rand16(), b = rand16();
    if (i & 1)
      b >>= (i & 15); // small divisors too
    uint16_t r;
    check("mul16", (uint16_t)(a * b), ref_mul16(a, b), a, b);
    if (b) {
      uint16_t q = ref_udiv16(a, b, &r);
      check("udiv16", a / b, q, a, b);
      check("umod16", a % b, r, a, b);
      int16_t sa = (int16_t)a, sb = (int16_t)b;
      if (!(sa == -32768 && sb == -1)) {
        uint16_t ua = sa < 0 ? -sa : sa, ub = sb < 0 ? -sb : sb;
        int16_t want_q = (int16_t)ref_udiv16(ua, ub, &r);
        int16_t want_r = (int16_t)r;
        if ((sa < 0) != (sb < 0))
          want_q = -want_q;
        if (sa < 0)
          want_r = -want_r;
        check("sdiv16", (uint16_t)(sa / sb), (uint16_t)want_q, a, b);
        check("smod16", (uint16_t)(sa % sb), (uint16_t)want_r, a, b);
      }
    }
  }
  for (int i = 0; i < 200; ++i) {
    uint32_t a = rand32(), b = rand32();
    if (i & 1)
      b = ref_lshr32(b, i & 31);
    uint32_t r;
    check("mul32", a * b, ref_mul32(a, b), a, b);
    if (b) {
      uint32_t q = ref_udiv32(a, b, &r);
      check("udiv32", a / b, q, a, b);
      check("umod32", a % b, r, a, b);
      int32_t sa = (int32_t)a, sb = (int32_t)b;
      uint32_t ua = sa < 0 ? -(uint32_t)sa : (uint32_t)sa;
      uint32_t ub = sb < 0 ? -(uint32_t)sb : (uint32_t)sb;
      int32_t want_q = (int32_t)ref_udiv32(ua, ub, &r);
      int32_t want_r = (int32_t)r;
      if ((sa < 0) != (sb < 0))
        want_q = -want_q;
      if (sa < 0)
        want_r = -want_r;
      if (!(sa == INT32_MIN && sb == -1)) {
        check("sdiv32", (uint32_t)(sa / sb), (uint32_t)want_q, a, b);
        check("smod32", (uint32_t)(sa % sb), (uint32_t)want_r, a, b);
      }
    }
    int n = rand16() & 31;
    check("shl32", a << n, ref_shl32(a, n), a, n);
    check("lshr32", a >> n, ref_lshr32(a, n), a, n);
    check("ashr32", (uint32_t)((int32_t)a >> n), (uint32_t)ref_ashr32(a, n),
          a, n);
  }
  // 64-bit, a few fixed cases.
  uint64_t x = 0x123456789ABCDEFULL, y = 0xFEDCBA987ULL;
  check("mul64.lo", (uint32_t)(x * y), 0x94116009u, 0, 0);
  check("mul64.hi", (uint32_t)((x * y) >> 32), 0xD70A3D5Fu, 0, 0);
  check("div64", (uint32_t)(x / y), 0x124924u, 0, 0);
  check("mod64.lo", (uint32_t)(x % y), 0x1A3277F3u, 0, 0);
  check("mod64.hi", (uint32_t)((x % y) >> 32), 0x9u, 0, 0);
  printf("%s: %d failures\n", failures ? "FAIL" : "PASS", failures);
  return failures != 0;
}
