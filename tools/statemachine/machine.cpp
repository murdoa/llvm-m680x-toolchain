// A small C++20 state machine, built for the host and for each target; run.py
// checks that every build prints the same thing.
//
// main prints run_machine's result for every event sequence of length 0, 1
// and 2 over all 256 byte values (Event has a fixed underlying type, so the
// values with no enumerator are valid too and leave the state alone). Each
// line is one first event: 256 digits, one per second event. The first
// lines are the empty sequence and the single events. The last line checks
// the compile-time evaluate<>() of each pair of enumerators against the
// runtime result.
//
// run_machine is reached two ways: through a volatile function pointer, so
// it is called with a variable count, and directly with a constant count,
// which the optimizer may inline and unroll.

#include <stdint.h>

#ifdef HOST
#include <stdio.h>
static void put(char c) { putchar(c); }
#else
extern "C" int __board_putc(int c);
static void put(char c) { __board_putc(c); }
#endif

enum class State : uint8_t { Idle, Running, Finished };
enum class Event : uint8_t { Start, Tick, Reset };

constexpr State transition(State state, Event event) {
  switch (state) {
  case State::Idle:
    if (event == Event::Start)
      return State::Running;
    break;
  case State::Running:
    if (event == Event::Tick)
      return State::Finished;
    if (event == Event::Reset)
      return State::Idle;
    break;
  case State::Finished:
    if (event == Event::Reset)
      return State::Idle;
    break;
  }
  return state;
}

template <Event... Events> consteval State evaluate() {
  State state = State::Idle;
  ((state = transition(state, Events)), ...);
  return state;
}

struct Machine {
  State state = State::Idle;
  constexpr void dispatch(Event event) { state = transition(state, event); }
};

extern "C" int run_machine(volatile uint8_t *events, unsigned count) {
  Machine machine;
  for (unsigned i = 0; i < count; ++i)
    machine.dispatch(static_cast<Event>(events[i]));
  return static_cast<int>(machine.state);
}

static int (*volatile run_indirect)(volatile uint8_t *, unsigned) = run_machine;

static void digit(int v) { put(static_cast<char>('0' + v)); }

template <Event A, Event B> static void check_folded() {
  constexpr int folded = static_cast<int>(evaluate<A, B>());
  volatile uint8_t events[] = {static_cast<uint8_t>(A),
                               static_cast<uint8_t>(B)};
  put(folded == run_machine(events, 2) ? '.' : 'X');
}

template <Event A> static void check_folded_row() {
  check_folded<A, Event::Start>();
  check_folded<A, Event::Tick>();
  check_folded<A, Event::Reset>();
}

int main() {
  volatile uint8_t events[2];
  digit(run_indirect(events, 0));
  put('\n');
  for (unsigned a = 0; a < 256; ++a) {
    events[0] = static_cast<uint8_t>(a);
    digit(run_indirect(events, 1));
  }
  put('\n');
  for (unsigned a = 0; a < 256; ++a) {
    events[0] = static_cast<uint8_t>(a);
    for (unsigned b = 0; b < 256; ++b) {
      events[1] = static_cast<uint8_t>(b);
      int indirect = run_indirect(events, 2);
      int direct = run_machine(events, 2);
      digit(indirect == direct ? indirect : 9);
    }
    put('\n');
  }
  check_folded_row<Event::Start>();
  check_folded_row<Event::Tick>();
  check_folded_row<Event::Reset>();
  put('\n');
  return 0;
}
