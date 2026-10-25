#include "bits.h"
#include <math.h>

double fabs(double x) { return asdouble(asuint64(x) & ~SIGN_MASK); }
