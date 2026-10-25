#include <math.h>

double floor(double x) {
  double t = trunc(x);
  return t > x ? t - 1.0 : t;
}
