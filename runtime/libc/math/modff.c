// Rounds the double result.
#include <math.h>

float modff(float x, float *ip) {
  double i;
  float r = (float)modf(x, &i);
  *ip = (float)i;
  return r;
}
