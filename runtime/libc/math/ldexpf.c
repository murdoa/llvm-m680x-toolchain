// Rounds the double result.
#include <math.h>

float ldexpf(float x, int e) { return (float)ldexp(x, e); }
