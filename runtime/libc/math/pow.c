#include <math.h>

// Integer powers by squaring, everything else as exp(y log x).
double pow(double x, double y) {
  if (y == 0.0)
    return 1.0;
  if (isnan(x) || isnan(y))
    return x + y;
  double yi = trunc(y);
  int y_is_int = yi == y;
  if (y_is_int && fabs(y) <= 1024.0) {
    long n = (long)y;
    unsigned long m = n < 0 ? -(unsigned long)n : (unsigned long)n;
    double r = 1.0, b = x;
    for (; m; m >>= 1, b *= b)
      if (m & 1)
        r *= b;
    return n < 0 ? 1.0 / r : r;
  }
  if (x == 0.0)
    return y > 0.0 ? 0.0 : HUGE_VAL;
  if (x < 0.0) {
    if (!y_is_int)
      return (x - x) / (x - x);
    // A large integer y: odd only below 2^53.
    double r = exp(y * log(-x));
    return fmod(yi, 2.0) != 0.0 ? -r : r;
  }
  return exp(y * log(x));
}
