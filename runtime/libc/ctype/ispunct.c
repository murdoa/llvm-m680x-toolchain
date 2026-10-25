#include <ctype.h>

int ispunct(int c) { return isprint(c) && !isalnum(c) && c != ' '; }
