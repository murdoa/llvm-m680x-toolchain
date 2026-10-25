#include <stdlib.h>

ldiv_t ldiv(long n, long d) {
  ldiv_t r = {n / d, n % d};
  return r;
}
