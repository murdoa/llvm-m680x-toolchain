// Rounds the double result.
#include <math.h>

float frexpf(float x, int *e) { return (float)frexp(x, e); }
