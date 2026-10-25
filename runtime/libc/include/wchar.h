#ifndef _WCHAR_H
#define _WCHAR_H
/* Only the types: there is no wide-character or multibyte support. */
#include <stddef.h>
typedef struct { unsigned char __state[4]; } mbstate_t;
typedef long wint_t;
#define WEOF ((wint_t)-1)
#endif
