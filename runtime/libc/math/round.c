#include <math.h>

// Halfway cases round away from zero.
double round(double x) {
  double t = trunc(x);
  double d = x - t;
  if (d >= 0.5)
    return t + 1.0;
  if (d <= -0.5)
    return t - 1.0;
  return t;
}
