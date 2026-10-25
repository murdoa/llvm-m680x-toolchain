#include <stdlib.h>

lldiv_t lldiv(long long n, long long d) {
  lldiv_t r = {n / d, n % d};
  return r;
}
