#include "bits.h"
#include <math.h>

double frexp(double x, int *e) {
  uint64_t u = asuint64(x);
  int be = biased_exponent(u);
  if (be == 0) {
    if (x == 0.0) {
      *e = 0;
      return x;
    }
    // Subnormal: scale into the normal range first.
    x = frexp(x * 0x1p64, e);
    *e -= 64;
    return x;
  }
  if (be == 0x7FF) {
    *e = 0;
    return x;
  }
  *e = be - (EXP_BIAS - 1);
  return asdouble((u & ~((uint64_t)0x7FF << MANT_BITS)) |
                  ((uint64_t)(EXP_BIAS - 1) << MANT_BITS));
}
