#include <stdlib.h>

div_t div(int n, int d) {
  div_t r = {n / d, n % d};
  return r;
}
