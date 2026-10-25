#include <math.h>

// Newton's method from a guess with the right exponent. Each step doubles
// the correct bits; six steps from a 4-bit guess are enough for 53.
double sqrt(double x) {
  if (x < 0.0)
    return (x - x) / (x - x);
  if (x == 0.0 || isinf(x) || isnan(x))
    return x;
  int e;
  double m = frexp(x, &e); // x = m * 2^e, m in [0.5, 1)
  if (e & 1) {
    m *= 2.0;
    --e;
  }
  // m in [0.5, 2): a linear guess at sqrt(m).
  double r = 0.41731 + 0.59016 * m;
  for (int i = 0; i < 6; ++i)
    r = 0.5 * (r + m / r);
  return ldexp(r, e / 2);
}
