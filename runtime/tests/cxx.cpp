// Freestanding C++ without exceptions or RTTI: global constructors and
// destructors, virtual calls, templates, lambdas, new and delete, and
// function-local statics.
#include <stdio.h>
#include <stdlib.h>

static int failures;
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      printf("cxx: line %d: %s\n", __LINE__, #c);                              \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

// A global with a constructor runs before main, and its destructor after
// main returns: main returns 9 when the rest passed, and only the
// destructor turns that into a 0 exit status.
static int constructed;
static bool passed;
struct Global {
  int v;
  Global() : v(42) { ++constructed; }
  ~Global() { _exit(passed && constructed == 1 ? 0 : 3); }
};
static Global global;

struct Shape {
  virtual ~Shape() {}
  virtual int area() const = 0;
  virtual const char *name() const { return "shape"; }
};

struct Rect : Shape {
  int w, h;
  Rect(int w, int h) : w(w), h(h) {}
  int area() const override { return w * h; }
  const char *name() const override { return "rect"; }
};

struct Square : Rect {
  explicit Square(int s) : Rect(s, s) {}
  const char *name() const override { return "square"; }
};

static int destroyed;
struct Counted {
  int v;
  explicit Counted(int v) : v(v) {}
  ~Counted() { ++destroyed; }
};

template <typename T, int N> struct Stack {
  T items[N];
  int n = 0;
  bool push(const T &t) {
    if (n == N)
      return false;
    items[n++] = t;
    return true;
  }
  T pop() { return items[--n]; }
};

template <typename T> T maxOf(T a, T b) { return a < b ? b : a; }

template <typename F> int apply(F f, int x) { return f(x); }

static int counter() {
  static int calls = 0; // a constant initialiser, no guard
  return ++calls;
}

static int sequence() {
  static Counted c(rand() % 1 + 7); // dynamic initialiser, guarded
  return c.v++;
}

constexpr int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }
static_assert(fact(7) == 5040, "constexpr");

int main() {
  CHECK(constructed == 1);
  CHECK(global.v == 42);

  Shape *shapes[] = {new Rect(3, 4), new Square(5)};
  CHECK(shapes[0]->area() == 12);
  CHECK(shapes[1]->area() == 25);
  CHECK(shapes[0]->name()[0] == 'r');
  CHECK(shapes[1]->name()[0] == 's');
  for (Shape *s : shapes)
    delete s;

  Counted *many = new Counted[3]{Counted(1), Counted(2), Counted(3)};
  CHECK(many[2].v == 3);
  delete[] many;
  CHECK(destroyed == 3);

  Stack<long, 4> st;
  for (long i = 1; st.push(i * 100000); ++i)
    ;
  CHECK(st.n == 4);
  CHECK(st.pop() == 400000);
  CHECK(maxOf<unsigned char>(200, 100) == 200);
  CHECK(maxOf(-3, -5) == -3);

  int base = 10;
  auto add = [base](int x) { return x + base; };
  auto twice = [&](int x) { return add(add(x)); };
  CHECK(apply(twice, 1) == 21);

  CHECK(counter() == 1 && counter() == 2);
  CHECK(sequence() == 7 && sequence() == 8);
  CHECK(fact(5) == 120);

  if (failures)
    return 1;
  printf("cxx: PASS\n");
  passed = true;
  return 9;
}
