#include "bits.h"
#include <math.h>

// The remainder is exact: shift-and-subtract on the mantissas.
double fmod(double x, double y) {
  uint64_t ux = asuint64(x), uy = asuint64(y);
  int ex = biased_exponent(ux), ey = biased_exponent(uy);
  uint64_t sx = ux & SIGN_MASK;

  if (ex == 0x7FF || isnan(y) || y == 0.0)
    return (x * y) / (x * y);
  ux &= ~SIGN_MASK;
  uy &= ~SIGN_MASK;
  if (ux <= uy)
    return ux == uy ? asdouble(sx) : x;

  // Normalize both to a mantissa with the implicit bit at bit 52.
  if (ex == 0) {
    for (uint64_t t = ux << 12; !(t >> 63); t <<= 1)
      --ex;
    ux <<= 1 - ex;
  } else {
    ux = (ux & MANT_MASK) | ((uint64_t)1 << MANT_BITS);
  }
  if (ey == 0) {
    for (uint64_t t = uy << 12; !(t >> 63); t <<= 1)
      --ey;
    uy <<= 1 - ey;
  } else {
    uy = (uy & MANT_MASK) | ((uint64_t)1 << MANT_BITS);
  }

  for (; ex > ey; --ex) {
    if (ux >= uy)
      ux -= uy;
    ux <<= 1;
  }
  if (ux >= uy)
    ux -= uy;
  if (ux == 0)
    return asdouble(sx);
  for (; !(ux >> MANT_BITS); ux <<= 1)
    --ex;
  if (ex > 0)
    ux = (ux - ((uint64_t)1 << MANT_BITS)) | ((uint64_t)ex << MANT_BITS);
  else
    ux >>= 1 - ex;
  return asdouble(ux | sx);
}
