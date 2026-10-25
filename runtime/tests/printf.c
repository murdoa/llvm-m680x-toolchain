// The small printf, checked through snprintf.

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(want, ...)                                                       \
  do {                                                                         \
    char buf[64];                                                              \
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
  char small[4];
  int n = snprintf(small, sizeof small, "%s", "truncate");
  if (n != 8 || strcmp(small, "tru")) {
    printf("truncation: %d \"%s\"\n", n, small);
    ++failures;
  }
  printf("%s: %d failures\n", failures ? "FAIL" : "PASS", failures);
  return failures != 0;
}
