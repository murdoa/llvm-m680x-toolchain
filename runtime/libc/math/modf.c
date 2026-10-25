#include <math.h>

double modf(double x, double *ip) {
  double t = trunc(x);
  *ip = t;
  if (isinf(x))
    return copysign(0.0, x);
  return copysign(x - t, x);
}
