// The small printf, checked through snprintf.

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(want, ...)                                                       \
  do {                                                                         \
    char buf[80];                                                              \
    snprintf(buf, sizeof buf, __VA_ARGS__);                                    \
    if (strcmp(buf, want)) {                                                   \
      printf("got \"%s\", want \"%s\"\n", buf, want);                          \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

int main(void) {
  CHECK("42", "%d", 42);
  CHECK("-32768", "%d", -32767 - 1);
  CHECK("65535", "%u", 65535u);
  CHECK("ffff", "%x", 0xFFFF);
  CHECK("0XAB", "%#X", 0xAB);
  CHECK("   7|7   |007", "%4d|%-4d|%03d", 7, 7, 7);
  CHECK("+5 -5", "%+d %+d", 5, -5);
  CHECK("2147483647", "%ld", 2147483647L);
  CHECK("-2147483648", "%ld", (long)-2147483647L - 1);
  CHECK("deadbeef", "%lx", 0xDEADBEEFUL);
  CHECK("18446744073709551615", "%llu", 18446744073709551615ULL);
  CHECK("-9223372036854775807", "%lld", -9223372036854775807LL);
  CHECK("hello|  hi|he", "%s|%4s|%.2s", "hello", "hi", "hello");
  CHECK("x%y", "%c%%%c", 'x', 'y');
  CHECK("0x1234", "%p", (void *)0x1234);
  CHECK("017", "%#o", 15);
  CHECK("00042", "%.5d", 42);
  CHECK("5", "%.0f", 5.0);
  CHECK("3.141593", "%f", 3.14159265358979);
  CHECK("-0.50|0.001|1.00e+03", "%.2f|%.3f|%.2e", -0.5, 0.00149, 999.5);
  CHECK("1e+100|1.5E-07", "%.0e|%.1E", 1e100, 1.5e-7);
  CHECK("100000|1e+06|0.0001|1e-05", "%g|%g|%g|%g", 1e5, 1e6, 1e-4, 1e-5);
  CHECK("3.14159|2.5|0|-0", "%g|%g|%g|%g", 3.14159265, 2.5, 0.0, -0.0);
  CHECK("  1.50|1.50  |001.50|+1.5", "%6.2f|%-6.2f|%06.2f|%+g", 1.5, 1.5,
        1.5, 1.5);
  CHECK("inf|-INF|nan", "%f|%E|%g", 1.0 / 0.0, -1.0 / 0.0, 0.0 / 0.0);
  CHECK("2.|2.00000", "%#.0f|%#g", 2.0, 2.0);
  CHECK("9.9|10|1e+02", "%.2g|%.2g|%.2g", 9.94, 9.96, 99.5);
  CHECK("0.12|0.38|2|4", "%.2f|%.2f|%.0f|%.0f", 0.125, 0.375, 2.5, 3.5);
  CHECK("123456789012345680000000", "%.0f", 1.23456789012345678e23);
  // Outside 1e-6 to 1e38 the scaling is not exact: 15 digits are right.
  CHECK("1.79769313486232e+308|4.94065645841247e-324", "%.14e|%.14e",
        1.7976931348623157e308, 4.9406564584124654e-324);
  char small[4];
  int n = snprintf(small, sizeof small, "%s", "truncate");
  if (n != 8 || strcmp(small, "tru")) {
    printf("truncation: %d \"%s\"\n", n, small);
    ++failures;
  }
  printf("%s: %d failures\n", failures ? "FAIL" : "PASS", failures);
  return failures != 0;
}
