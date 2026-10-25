#include <math.h>

double ceil(double x) {
  double t = trunc(x);
  return t < x ? t + 1.0 : t;
}
