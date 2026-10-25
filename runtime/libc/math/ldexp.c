#include <math.h>

double ldexp(double x, int e) { return scalbn(x, e); }
