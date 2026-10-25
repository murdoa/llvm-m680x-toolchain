#include "bits.h"
#include <math.h>

// Scales in steps that stay normal; the last multiply rounds once.
double scalbn(double x, int e) {
  while (e > 1023) {
    x *= 0x1p1023;
    e -= 1023;
    if (isinf(x))
      return x;
  }
  while (e < -1022) {
    x *= 0x1p-969;
    e += 969;
    if (x == 0.0)
      return x;
  }
  return x * asdouble((uint64_t)(e + EXP_BIAS) << MANT_BITS);
}
