#include <math.h>

// x = m * 2^e with m in [sqrt(1/2), sqrt(2)), and
// log(m) = 2 atanh(s) = 2 (s + s^3/3 + s^5/5 + ...) with s = (m-1)/(m+1).
double log(double x) {
  if (isnan(x) || x == HUGE_VAL)
    return x;
  if (x < 0.0)
    return (x - x) / (x - x);
  if (x == 0.0)
    return -HUGE_VAL;
  int e;
  double m = frexp(x, &e);
  if (m < M_SQRT2 / 2) {
    m *= 2.0;
    --e;
  }
  double s = (m - 1.0) / (m + 1.0), s2 = s * s;
  double term = s, sum = 0.0;
  for (int n = 1; n < 40; n += 2) {
    sum += term / n;
    term *= s2;
  }
  return 2.0 * sum + e * M_LN2;
}
