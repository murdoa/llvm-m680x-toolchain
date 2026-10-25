#ifndef _SETJMP_H
#define _SETJMP_H
#ifdef __cplusplus
extern "C" {
#endif
// U (the only callee-saved register), S after the return, and the return
// address.
typedef unsigned short jmp_buf[3];
int setjmp(jmp_buf env) __attribute__((returns_twice));
_Noreturn void longjmp(jmp_buf env, int val);
#define _setjmp setjmp
#define _longjmp longjmp
#ifdef __cplusplus
}
#endif
#endif
