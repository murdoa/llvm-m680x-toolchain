/* Mask interrupts around an atomic operation.  There is one CPU, so that
   is all atomicity takes. */
#ifndef ATOMIC_IRQ_H
#define ATOMIC_IRQ_H
#include <stdint.h>

#if defined(__M6809__)
typedef uint8_t irqstate;
static inline irqstate irq_save(void) {
  irqstate cc;
  __asm__ volatile("tfr cc,%0\n\torcc #$50" : "=q"(cc) : : "memory");
  return cc;
}
static inline void irq_restore(irqstate cc) {
  __asm__ volatile("tfr %0,cc" : : "q"(cc) : "memory");
}
#elif defined(__M6800__)
typedef uint8_t irqstate;
static inline irqstate irq_save(void) {
  register irqstate cc __asm__("a");
  __asm__ volatile("tpa\n\tsei" : "=r"(cc) : : "memory");
  return cc;
}
static inline void irq_restore(irqstate s) {
  register irqstate cc __asm__("a") = s;
  __asm__ volatile("tap" : : "r"(cc) : "memory");
}
#else
#error "unknown target"
#endif

#endif
