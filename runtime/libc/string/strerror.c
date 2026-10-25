#include <errno.h>
#include <string.h>

char *strerror(int errnum) {
  switch (errnum) {
  case 0:
    return (char *)"No error";
  case EDOM:
    return (char *)"Argument out of domain";
  case ERANGE:
    return (char *)"Result out of range";
  case EINVAL:
    return (char *)"Invalid argument";
  case ENOMEM:
    return (char *)"Out of memory";
  case ENOSYS:
    return (char *)"Not implemented";
  case EOVERFLOW:
    return (char *)"Value too large";
  case EILSEQ:
    return (char *)"Illegal byte sequence";
  default:
    return (char *)"Unknown error";
  }
}
