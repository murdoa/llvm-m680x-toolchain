// The parts of the C++ ABI that code built without exceptions or RTTI
// still calls.

#include <stdlib.h>

extern "C" {

// A pure virtual function was called: a bug in the program.
void __cxa_pure_virtual() { abort(); }

// Guards for function-local statics. There is one thread, so the guard's
// first byte only records that initialisation finished.
int __cxa_guard_acquire(char *guard) { return !*guard; }
void __cxa_guard_release(char *guard) { *guard = 1; }
void __cxa_guard_abort(char *) {}

} // extern "C"
