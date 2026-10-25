#include "bits.h"
#include <math.h>

double copysign(double x, double y) {
  return asdouble((asuint64(x) & ~SIGN_MASK) | (asuint64(y) & SIGN_MASK));
}
