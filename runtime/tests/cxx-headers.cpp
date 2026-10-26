// The C++ standard headers that freestanding programs for these CPUs use,
// each exercised at run time with -std=c++26.
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <chrono>
#include <compare>
#include <concepts>
#include <cstdint>
#include <expected>
#include <functional>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <stdio.h>

// Failures are reported with puts and to_chars rather than printf, which
// would bring in floating-point formatting and leave the 6800 short of room.
static int failures;
static void fail(int line, const char *what) {
  char buf[8];
  *std::to_chars(buf, buf + sizeof buf - 1, line).ptr = '\0';
  fputs("cxx-headers: line ", stdout);
  fputs(buf, stdout);
  fputs(": ", stdout);
  puts(what);
  ++failures;
}
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c))                                                                  \
      fail(__LINE__, #c);                                                      \
  } while (0)

// Keep the optimizer from folding everything to constants.
template <typename T> static T opaque(T v) {
  asm volatile("" : "+m"(v));
  return v;
}

static void testArraySpanAlgorithm() {
  std::array<int, 6> a{5, -3, 9, 0, 7, -3};
  std::ranges::sort(a);
  CHECK((a == std::array<int, 6>{-3, -3, 0, 5, 7, 9}));
  std::sort(a.begin(), a.end(), std::greater<>{});
  CHECK(a.front() == 9 && a.back() == -3);
  std::span<const int> s(a);
  CHECK(s.size() == 6 && s.subspan(1, 2)[1] == 5);
  CHECK(std::ranges::count(s, -3) == 2);
  CHECK(*std::ranges::max_element(s) == 9);
  CHECK(std::accumulate(s.begin(), s.end(), 0) == 15);
  std::array<long, 4> p{};
  std::iota(p.begin(), p.end(), opaque(100000L));
  CHECK(std::reduce(p.begin(), p.end()) == 400006L);
  CHECK(std::gcd(opaque(84), 36) == 12 && std::lcm(opaque(4), 6) == 12);
}

static void testOptionalExpectedVariant() {
  auto half = [](int v) -> std::optional<int> {
    if (v % 2)
      return std::nullopt;
    return v / 2;
  };
  CHECK(half(opaque(10)) == 5);
  CHECK(!half(opaque(7)));
  CHECK(half(opaque(8)).and_then(half).value_or(-1) == 2);

  enum class Err { tooBig };
  auto check = [](int v) -> std::expected<int, Err> {
    if (v > 100)
      return std::unexpected(Err::tooBig);
    return v + 1;
  };
  CHECK(check(opaque(5)).value() == 6);
  CHECK(check(opaque(500)).error() == Err::tooBig);
  CHECK(check(opaque(1)).transform([](int v) { return v * 3; }) == 6);

  std::variant<int, long, std::string_view> v = opaque(3);
  CHECK(std::holds_alternative<int>(v));
  v = std::string_view("six");
  CHECK(std::visit(
            [](auto x) -> int {
              if constexpr (std::is_same_v<decltype(x), std::string_view>)
                return int(x.size());
              else
                return int(x);
            },
            v) == 3);
  v = 70000L;
  CHECK(std::get<long>(v) == 70000L && v.index() == 1);
}

static void testTupleUtilityFunctional() {
  auto t = std::tuple(opaque(1), 2L, 'c');
  auto [i, l, c] = t;
  CHECK(i == 1 && l == 2L && c == 'c');
  CHECK(std::apply([](int a, long b, char) { return a + b; }, t) == 3);
  auto p = std::pair(opaque(4), 5);
  std::swap(p.first, p.second);
  CHECK(p.first == 5 && std::exchange(p.second, 9) == 4 && p.second == 9);
  std::function<int(int)> f = [k = opaque(7)](int x) { return x * k; };
  CHECK(f(6) == 42);
  CHECK(std::invoke(std::plus<>{}, 2, 3) == 5);
  auto add5 = std::bind_front(std::plus<int>{}, 5);
  CHECK(add5(opaque(1)) == 6);
}

static void testBitLimitsTraits() {
  CHECK(std::popcount(opaque(0xF0F0u)) == 8);
  CHECK(std::countl_zero(opaque(std::uint16_t(1))) == 15);
  CHECK(std::bit_width(opaque(1000u)) == 10);
  CHECK(std::rotl(opaque(std::uint8_t(0x81)), 1) == 0x03);
  CHECK(std::byteswap(opaque(std::uint16_t(0x1234))) == 0x3412);
  CHECK(std::bit_cast<std::uint32_t>(opaque(1.0f)) == 0x3F800000u);
  CHECK(std::endian::native == std::endian::big);
  static_assert(std::numeric_limits<int>::max() == 32767);
  static_assert(std::numeric_limits<long>::digits == 31);
  static_assert(std::is_trivially_copyable_v<std::array<int, 3>>);
  static_assert(std::is_same_v<std::make_unsigned_t<long>, unsigned long>);
  CHECK(std::numeric_limits<float>::epsilon() > 0.0f);
}

template <std::integral T> static constexpr T twice(T v) { return v * 2; }
template <typename T>
concept Shape = requires(const T &s) {
  { s.area() } -> std::convertible_to<int>;
};
struct Square {
  int side;
  int area() const { return side * side; }
};
template <Shape S> static int area(const S &s) { return s.area(); }

struct Version {
  int major, minor;
  auto operator<=>(const Version &) const = default;
};

static void testConceptsCompare() {
  CHECK(twice(opaque(21)) == 42);
  CHECK(area(Square{opaque(5)}) == 25);
  static_assert(!Shape<int>);
  CHECK((Version{1, 2} < Version{1, opaque(3)}));
  CHECK((Version{2, 0} <=> Version{opaque(1), 9}) > 0);
  CHECK(std::strong_order(opaque(1), 2) == std::strong_ordering::less);
}

static void testRangesIterator() {
  std::array<int, 8> a{1, 2, 3, 4, 5, 6, 7, 8};
  int sum = 0;
  for (int v : a | std::views::filter([](int x) { return x % 2 == 0; }) |
                   std::views::transform([](int x) { return x * x; }))
    sum += v;
  CHECK(sum == 4 + 16 + 36 + 64);
  auto r = std::views::reverse(a) | std::views::take(3);
  CHECK(*std::ranges::begin(r) == 8 && std::ranges::distance(r) == 3);
  CHECK(std::distance(a.begin(), std::next(a.begin(), 5)) == 5);
  std::array<int, 3> out{};
  std::ranges::copy(std::views::iota(opaque(10), 13), out.begin());
  CHECK((out == std::array{10, 11, 12}));
}

static void testStringViewCharconv() {
  std::string_view s = "volts=-12.5";
  auto eq = s.find('=');
  CHECK(eq == 5 && s.substr(0, eq) == "volts" && s.starts_with("vo"));
  int v = 0;
  auto [ptr, ec] = std::from_chars(s.data() + 6, s.data() + s.size(), v);
  CHECK(ec == std::errc{} && v == -12 && *ptr == '.');
  char buf[12];
  auto res = std::to_chars(buf, buf + sizeof buf, opaque(-31416));
  CHECK(std::string_view(buf, res.ptr) == "-31416");
  res = std::to_chars(buf, buf + sizeof buf, opaque(0xBEEFu), 16);
  CHECK(std::string_view(buf, res.ptr) == "beef");
  long l = 0;
  std::from_chars(buf, res.ptr, l, 16);
  CHECK(l == 0xBEEFL);
}

static void testAtomicChrono() {
  std::atomic<int> a{opaque(5)};
  CHECK(a.fetch_add(3) == 5 && a.load() == 8);
  int expected = 8;
  CHECK(a.compare_exchange_strong(expected, 1) && a == 1);
  std::atomic_flag f;
  CHECK(!f.test_and_set() && f.test_and_set());
  using namespace std::chrono;
  milliseconds ms = opaque(1500) * 1ms;
  CHECK(duration_cast<seconds>(ms).count() == 1);
  CHECK(minutes(opaque(3)) == 180s);
  CHECK(duration_cast<seconds>(days(opaque(2))).count() == 172800);
  year_month_day d{sys_days{days{opaque(20751)}}};
  CHECK(d == 2026y / October / 25);
  CHECK(sys_days{2000y / March / 1}.time_since_epoch().count() == 11017);
}

int main() {
  testArraySpanAlgorithm();
  testOptionalExpectedVariant();
  testTupleUtilityFunctional();
  testBitLimitsTraits();
  testConceptsCompare();
  testRangesIterator();
  testStringViewCharconv();
  testAtomicChrono();
  puts(failures ? "cxx-headers: FAIL" : "cxx-headers: PASS");
  return failures != 0;
}
