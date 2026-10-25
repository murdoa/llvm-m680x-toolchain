// The pieces of the C library that libc++ needs: errno, strerror, lldiv,
// remove and rename (which fail: there are no files), and the types in
// wchar.h and time.h.
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

static int failures;
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      printf("libc-extra: line %d: %s\n", __LINE__, #c);                       \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

int main(void) {
  lldiv_t q = lldiv(-7000000000LL, 3);
  CHECK(q.quot == -2333333333LL && q.rem == -1);
  errno = 0;
  CHECK(remove("x") == -1 && errno == ENOSYS);
  errno = 0;
  CHECK(rename("x", "y") == -1 && errno == ENOSYS);
  CHECK(strcmp(strerror(ERANGE), "Result out of range") == 0);
  CHECK(strcmp(strerror(12345), "Unknown error") == 0);
  mbstate_t st = {{0}};
  CHECK(sizeof st == 4 && WEOF == -1);
  time_t t = 1;
  struct timespec ts = {t, 0};
  CHECK(sizeof(time_t) == 8 && ts.tv_sec == 1);
  if (!failures)
    puts("libc-extra: PASS");
  return failures != 0;
}
