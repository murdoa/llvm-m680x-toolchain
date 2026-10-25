// C11 atomics: 8- and 16-bit loads and stores are plain instructions,
// read-modify-write operations call __sync_*, wider ones __atomic_*.
// Misaligned and 32-bit ones go to the __atomic_* library, as intended.
#pragma clang diagnostic ignored "-Watomic-alignment"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

static _Atomic uint8_t b = 200;
static _Atomic int16_t w = -5;

#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      printf("atomic: line %d\n", __LINE__);                                   \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main(void) {
  CHECK(atomic_fetch_add(&b, 100) == 200 && atomic_load(&b) == 44);
  CHECK(atomic_fetch_sub(&b, 45) == 44 && b == 255);
  CHECK(atomic_fetch_and(&b, 0x0F) == 255 && b == 0x0F);
  CHECK(atomic_fetch_or(&b, 0x30) == 0x0F && b == 0x3F);
  CHECK(atomic_fetch_xor(&b, 0x11) == 0x3F && b == 0x2E);
  CHECK(atomic_exchange(&b, 7) == 0x2E && b == 7);
  uint8_t e = 6;
  CHECK(!atomic_compare_exchange_strong(&b, &e, 9) && e == 7 && b == 7);
  CHECK(atomic_compare_exchange_strong(&b, &e, 9) && b == 9);

  CHECK(atomic_fetch_add(&w, 1000) == -5 && w == 995);
  CHECK(atomic_fetch_sub_explicit(&w, 2000, memory_order_relaxed) == 995);
  CHECK(atomic_load_explicit(&w, memory_order_acquire) == -1005);
  atomic_store_explicit(&w, 0x1234, memory_order_release);
  CHECK(atomic_fetch_and(&w, 0x0FF0) == 0x1234 && w == 0x0230);
  int16_t f = 0x0230;
  CHECK(atomic_compare_exchange_weak(&w, &f, -1) && w == -1);
  CHECK(w == -1);
  static int16_t m = -1;
  CHECK(__atomic_fetch_max(&m, 3, __ATOMIC_SEQ_CST) == -1 && m == 3);
  CHECK(__atomic_fetch_min((uint16_t *)&m, 2, __ATOMIC_SEQ_CST) == 3);
  CHECK(m == 2);
  static int32_t l = 70000;
  CHECK(__atomic_fetch_add(&l, 1, __ATOMIC_SEQ_CST) == 70000 && l == 70001);
  atomic_thread_fence(memory_order_seq_cst);
  atomic_signal_fence(memory_order_seq_cst);

  printf("atomic: PASS\n");
  return 0;
}
