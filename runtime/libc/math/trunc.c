#include "bits.h"
#include <math.h>

// Clears the fraction bits. Values of 2^52 and up are already integers.
double trunc(double x) {
  uint64_t u = asuint64(x);
  int e = biased_exponent(u) - EXP_BIAS;
  if (e >= MANT_BITS)
    return x;
  if (e < 0)
    return asdouble(u & SIGN_MASK);
  return asdouble(u & ~(MANT_MASK >> e));
}
