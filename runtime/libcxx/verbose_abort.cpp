// libc++ reports a failed check, or an exception thrown with exceptions
// turned off, here before aborting. Its own version formats the message with
// vfprintf, which would bring printf and floating point into any program
// that calls optional::value or variant's get. The messages only use %s, so
// this prints them with fputs.

#include <__config>
#include <__verbose_abort>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

_LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_EXPLICIT_ABI_ANNOTATIONS

void __libcpp_verbose_abort(char const *format, ...) noexcept {
  va_list args;
  va_start(args, format);
  for (const char *p = format; *p; ++p) {
    if (p[0] == '%' && p[1] == 's') {
      std::fputs(va_arg(args, const char *), stderr);
      ++p;
    } else {
      std::fputc(*p, stderr);
    }
  }
  va_end(args);
  std::fputc('\n', stderr);
  std::abort();
}

_LIBCPP_END_EXPLICIT_ABI_ANNOTATIONS
_LIBCPP_END_NAMESPACE_STD
