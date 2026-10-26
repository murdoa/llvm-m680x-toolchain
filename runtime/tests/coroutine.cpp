// C++20 coroutines without a heap: frames come from a fixed arena through
// the promise's operator new. A generator and a task awaiting events.
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <stdio.h>

static int failures;
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      puts("coroutine: failed: " #c);                                          \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

alignas(2) static unsigned char arena[512];
static std::size_t used, frames;

struct Arena {
  static void *operator new(std::size_t n) noexcept {
    if (used + n > sizeof arena)
      return nullptr;
    void *p = arena + used;
    used += (n + 1) & ~std::size_t(1);
    ++frames;
    return p;
  }
  static void operator delete(void *, std::size_t) noexcept { --frames; }
};

template <typename T> struct Generator {
  struct promise_type : Arena {
    std::optional<T> value;
    static Generator get_return_object_on_allocation_failure() { return {}; }
    Generator get_return_object() {
      return {std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    std::suspend_always yield_value(T v) {
      value = v;
      return {};
    }
    void return_void() {}
    void unhandled_exception() {}
  };
  std::coroutine_handle<promise_type> h;
  std::optional<T> next() {
    h.promise().value.reset();
    if (!h.done())
      h.resume();
    return h.promise().value;
  }
  ~Generator() {
    if (h)
      h.destroy();
  }
};

static Generator<long> fibonacci(int n) {
  long a = 0, b = 1;
  for (int i = 0; i < n; ++i) {
    co_yield a;
    long t = a + b;
    a = b;
    b = t;
  }
}

// A task that waits for an event the main loop delivers.
static std::coroutine_handle<> waiting;
static int event_value;

struct Event {
  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<> h) { waiting = h; }
  int await_resume() { return event_value; }
};

struct Task {
  struct promise_type : Arena {
    static Task get_return_object_on_allocation_failure() { return {}; }
    Task get_return_object() {
      return {std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
  std::coroutine_handle<promise_type> h;
};

static int total;
static Task adder() {
  for (;;) {
    int v = co_await Event{};
    if (v < 0)
      co_return;
    total += v;
  }
}

static void deliver(int v) {
  event_value = v;
  auto h = waiting;
  waiting = nullptr;
  h.resume();
}

int main() {
  {
    auto g = fibonacci(20);
    long last = 0;
    int count = 0;
    while (auto v = g.next()) {
      last = *v;
      ++count;
    }
    CHECK(count == 20 && last == 4181);
    CHECK(frames == 1);
  }
  CHECK(frames == 0);

  Task t = adder();
  CHECK(waiting);
  for (int i = 1; i <= 10; ++i)
    deliver(i);
  CHECK(total == 55);
  deliver(-1);
  CHECK(t.h.done());
  t.h.destroy();
  CHECK(frames == 0);

  puts(failures ? "coroutine: FAIL" : "coroutine: PASS");
  return failures != 0;
}
