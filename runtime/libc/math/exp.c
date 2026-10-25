#include <math.h>

// x = k*ln2 + r with |r| <= ln2/2, then e^r from its Taylor series.
double exp(double x) {
  if (isnan(x))
    return x;
  if (x > 709.8)
    return HUGE_VAL;
  if (x < -745.2)
    return 0.0;
  double k = round(x / M_LN2);
  double r = x - k * M_LN2;
  double term = 1.0, sum = 1.0;
  for (int n = 1; n < 18; ++n) {
    term *= r / n;
    sum += term;
  }
  return ldexp(sum, (int)k);
}
