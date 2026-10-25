#include <stdlib.h>

static unsigned long rand_state = 1;

int rand(void) {
  rand_state = rand_state * 1103515245UL + 12345;
  return (int)((rand_state >> 16) & RAND_MAX);
}

void srand(unsigned seed) { rand_state = seed; }
