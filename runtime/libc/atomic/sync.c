/* The __sync_* functions the compiler calls for 8- and 16-bit atomic
   read-modify-write operations.  There is one CPU, so masking interrupts
   around the operation makes it atomic.  Clang reserves the __sync_ names
   in C, so the functions get them through asm labels. */
#include "irq.h"

#define SYNC_ALL(N, TY, STY)                                                   \
  SYNC_OP(N, TY, add, old + v)                                                 \
  SYNC_OP(N, TY, sub, old - v)                                                 \
  SYNC_OP(N, TY, and, old & v)                                                 \
  SYNC_OP(N, TY, or, old | v)                                                  \
  SYNC_OP(N, TY, xor, old ^ v)                                                 \
  SYNC_OP(N, TY, nand, ~(old & v))                                             \
  SYNC_OP(N, TY, max, (STY)old > (STY)v ? old : v)                             \
  SYNC_OP(N, TY, min, (STY)old < (STY)v ? old : v)                             \
  SYNC_OP(N, TY, umax, old > v ? old : v)                                      \
  SYNC_OP(N, TY, umin, old < v ? old : v)                                      \
  TY sync_lock_test_and_set_##N(volatile TY *p, TY v)                         \
      __asm__("__sync_lock_test_and_set_" #N);                                 \
  TY sync_lock_test_and_set_##N(volatile TY *p, TY v) {                       \
    irqstate s = irq_save();                                                   \
    TY old = *p;                                                               \
    *p = v;                                                                    \
    irq_restore(s);                                                            \
    return old;                                                                \
  }                                                                            \
  TY sync_val_compare_and_swap_##N(volatile TY *p, TY expected, TY desired)   \
      __asm__("__sync_val_compare_and_swap_" #N);                              \
  TY sync_val_compare_and_swap_##N(volatile TY *p, TY expected,               \
                                   TY desired) {                              \
    irqstate s = irq_save();                                                   \
    TY old = *p;                                                               \
    if (old == expected)                                                       \
      *p = desired;                                                            \
    irq_restore(s);                                                            \
    return old;                                                                \
  }

#define SYNC_OP(N, TY, name, expr)                                             \
  TY sync_fetch_and_##name##_##N(volatile TY *p, TY v)                        \
      __asm__("__sync_fetch_and_" #name "_" #N);                               \
  TY sync_fetch_and_##name##_##N(volatile TY *p, TY v) {                      \
    irqstate s = irq_save();                                                   \
    TY old = *p;                                                               \
    *p = (expr);                                                               \
    irq_restore(s);                                                            \
    return old;                                                                \
  }

SYNC_ALL(1, uint8_t, int8_t)
SYNC_ALL(2, uint16_t, int16_t)
