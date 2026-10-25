/* The __atomic_* library functions, for atomics wider than 16 bits or
   not aligned to their size.  Each runs with interrupts masked, which on
   one CPU makes it atomic whatever its size.  The memory order arguments
   are ignored.  Clang reserves the __atomic_ names in C, so the functions
   get them through asm labels. */
#include <stdbool.h>
#include <stddef.h>

#include "irq.h"

static void copy(volatile unsigned char *d, const volatile unsigned char *s,
                 size_t n) {
  while (n--)
    *d++ = *s++;
}

static bool same(const volatile unsigned char *a,
                 const volatile unsigned char *b, size_t n) {
  while (n--)
    if (*a++ != *b++)
      return false;
  return true;
}

#define NAMED(ret, name, args) ret atomic_##name args __asm__("__atomic_" #name);

NAMED(void, load, (size_t, const volatile void *, void *, int))
void atomic_load(size_t n, const volatile void *src, void *dst, int order) {
  (void)order;
  irqstate s = irq_save();
  copy(dst, src, n);
  irq_restore(s);
}

NAMED(void, store, (size_t, volatile void *, const void *, int))
void atomic_store(size_t n, volatile void *dst, const void *src, int order) {
  (void)order;
  irqstate s = irq_save();
  copy(dst, src, n);
  irq_restore(s);
}

NAMED(void, exchange, (size_t, volatile void *, const void *, void *, int))
void atomic_exchange(size_t n, volatile void *p, const void *val, void *old,
                     int order) {
  (void)order;
  irqstate s = irq_save();
  copy(old, p, n);
  copy(p, val, n);
  irq_restore(s);
}

NAMED(bool, compare_exchange,
      (size_t, volatile void *, void *, const void *, int, int))
bool atomic_compare_exchange(size_t n, volatile void *p, void *expected,
                             const void *desired, int ok, int fail) {
  (void)ok, (void)fail;
  irqstate s = irq_save();
  bool eq = same(p, expected, n);
  if (eq)
    copy(p, desired, n);
  else
    copy(expected, p, n);
  irq_restore(s);
  return eq;
}

#define RMW(N, T, name, expr)                                                  \
  NAMED(T, fetch_##name##_##N, (volatile T *, T, int))                         \
  T atomic_fetch_##name##_##N(volatile T *p, T v, int order) {                 \
    (void)order;                                                               \
    irqstate s = irq_save();                                                   \
    T old = *p;                                                                \
    *p = (expr);                                                               \
    irq_restore(s);                                                            \
    return old;                                                                \
  }

#define SIZED(N, T)                                                            \
  NAMED(T, load_##N, (const volatile T *, int))                                \
  T atomic_load_##N(const volatile T *p, int order) {                          \
    (void)order;                                                               \
    irqstate s = irq_save();                                                   \
    T v = *p;                                                                  \
    irq_restore(s);                                                            \
    return v;                                                                  \
  }                                                                            \
  NAMED(void, store_##N, (volatile T *, T, int))                               \
  void atomic_store_##N(volatile T *p, T v, int order) {                       \
    (void)order;                                                               \
    irqstate s = irq_save();                                                   \
    *p = v;                                                                    \
    irq_restore(s);                                                            \
  }                                                                            \
  NAMED(T, exchange_##N, (volatile T *, T, int))                               \
  T atomic_exchange_##N(volatile T *p, T v, int order) {                       \
    (void)order;                                                               \
    irqstate s = irq_save();                                                   \
    T old = *p;                                                                \
    *p = v;                                                                    \
    irq_restore(s);                                                            \
    return old;                                                                \
  }                                                                            \
  NAMED(bool, compare_exchange_##N, (volatile T *, T *, T, int, int))          \
  bool atomic_compare_exchange_##N(volatile T *p, T *expected, T desired,      \
                                   int ok, int fail) {                         \
    (void)ok, (void)fail;                                                      \
    irqstate s = irq_save();                                                   \
    T old = *p;                                                                \
    bool eq = old == *expected;                                                \
    if (eq)                                                                    \
      *p = desired;                                                            \
    else                                                                       \
      *expected = old;                                                         \
    irq_restore(s);                                                            \
    return eq;                                                                 \
  }                                                                            \
  RMW(N, T, add, old + v)                                                      \
  RMW(N, T, sub, old - v)                                                      \
  RMW(N, T, and, old & v)                                                      \
  RMW(N, T, or, old | v)                                                       \
  RMW(N, T, xor, old ^ v)                                                      \
  RMW(N, T, nand, ~(old & v))

SIZED(1, uint8_t)
SIZED(2, uint16_t)
SIZED(4, uint32_t)
SIZED(8, uint64_t)
