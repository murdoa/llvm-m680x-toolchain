// The math functions on a few exact cases, and the inexact ones to within
// a relative 1e-12.
#include <math.h>
#include <stdio.h>

static int failures;

static void exact(const char *what, double got, double want) {
  if (got != want && !(isnan(got) && isnan(want))) {
    printf("math: %s gave %ld/1000, want %ld/1000\n", what,
           (long)(got * 1000), (long)(want * 1000));
    ++failures;
  }
}

static void near(const char *what, double got, double want) {
  if (fabs(got - want) > 1e-12 * fabs(want)) {
    printf("math: %s gave %ld/1e6, want %ld/1e6\n", what,
           (long)(got * 1e6), (long)(want * 1e6));
    ++failures;
  }
}

int main(void) {
  volatile double v = 2.5, n = -2.5, big = 1e300, tiny = 1e-310;
  exact("floor(2.5)", floor(v), 2.0);
  exact("floor(-2.5)", floor(n), -3.0);
  exact("ceil(2.5)", ceil(v), 3.0);
  exact("ceil(-2.5)", ceil(n), -2.0);
  exact("trunc(-2.5)", trunc(n), -2.0);
  exact("round(2.5)", round(v), 3.0);
  exact("round(-2.5)", round(n), -3.0);
  exact("floor(1e300)", floor(big), big);
  exact("floor(0.1)", floor(v / 25), 0.0);
  exact("fabs(-2.5)", fabs(n), 2.5);
  exact("fmod(7.5, 2)", fmod(v * 3, 2.0), 1.5);
  exact("fmod(-7.5, 2)", fmod(n * 3, 2.0), -1.5);
  exact("fmod(1e300, 3)", fmod(big, 3.0), 0.0);
  exact("fmod(1e300, 7)", fmod(big, 7.0), 1.0);
  int e;
  exact("frexp(2.5)", frexp(v, &e), 0.625);
  exact("frexp(2.5) exponent", e, 2);
  exact("frexp(1e-310)", ldexp(frexp(tiny, &e), e), tiny);
  exact("ldexp(2.5, -1074)", ldexp(ldexp(v, -1074), 1074), 2.0);
  exact("sqrt(6.25)", sqrt(v * v), 2.5);
  exact("sqrt(2)^2", sqrt(2.0) * sqrt(2.0), 2.0000000000000004);
  near("exp(1)", exp(1.0), M_E);
  near("exp(-10)", exp(-10.0), 4.5399929762484854e-05);
  near("log(10)", log(10.0), M_LN10);
  near("log(1e-300)", log(1e-300), -690.77552789821368);
  near("pow(32, 1/3)", pow(32.0, 1.0 / 3.0), 3.1748021039363987);
  exact("pow(-2, 3)", pow(-2.0, 3.0), -8.0);
  exact("pow(2, -2)", pow(2.0, -2.0), 0.25);
  exact("floorf(-0.5)", floorf(-0.5f), -1.0f);
  if (failures)
    return 1;
  printf("math: PASS\n");
  return 0;
}
