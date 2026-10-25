// Access to the bits of a double: sign, 11-bit exponent, 52-bit mantissa.
#include <stdint.h>

static inline uint64_t asuint64(double x) {
  union {
    double d;
    uint64_t u;
  } v = {x};
  return v.u;
}

static inline double asdouble(uint64_t u) {
  union {
    uint64_t u;
    double d;
  } v = {u};
  return v.d;
}

#define EXP_BIAS 1023
#define MANT_BITS 52
#define SIGN_MASK ((uint64_t)1 << 63)
#define MANT_MASK (((uint64_t)1 << MANT_BITS) - 1)
static inline int biased_exponent(uint64_t u) {
  return (int)(u >> MANT_BITS) & 0x7FF;
}
