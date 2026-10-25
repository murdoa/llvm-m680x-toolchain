#ifndef _TIME_H
#define _TIME_H
/* Only the types: there is no clock. */
#include <stddef.h>
typedef long long time_t;
typedef long clock_t;
struct timespec {
  time_t tv_sec;
  long tv_nsec;
};
struct tm {
  int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday,
      tm_isdst;
};
#endif
