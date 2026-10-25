// setjmp/longjmp, from a nested call and with a value of 0.
#include <setjmp.h>
#include <stdio.h>

static jmp_buf env;
static int depth;

__attribute__((noinline)) static void down(int n) {
  char pad[5];
  pad[0] = n;
  ++depth;
  if (n == 0)
    longjmp(env, pad[0] + 42);
  down(n - 1);
}

int main(void) {
  volatile int calls = 0;
  int r = setjmp(env);
  ++calls;
  if (calls == 1)
    down(5);
  if (r != 42 || depth != 6) {
    printf("setjmp: r=%d depth=%d\n", r, depth);
    return 1;
  }
  if (setjmp(env) == 0)
    longjmp(env, 0);
  else
    calls += 10;
  if (calls != 12) {
    printf("setjmp: longjmp(env, 0) calls=%d\n", (int)calls);
    return 1;
  }
  printf("setjmp: PASS\n");
  return 0;
}
