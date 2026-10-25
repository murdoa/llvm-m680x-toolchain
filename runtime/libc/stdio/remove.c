#include <errno.h>
#include <stdio.h>

int remove(const char *path) {
  (void)path;
  errno = ENOSYS;
  return -1;
}

int rename(const char *from, const char *to) {
  (void)from;
  (void)to;
  errno = ENOSYS;
  return -1;
}
