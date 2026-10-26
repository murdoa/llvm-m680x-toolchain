// std::sort of one built-in type, TYPE, which the Makefile sets. libc++'s
// algorithm.cpp instantiates the sorts of all of them in one object, so a
// program sorting ints would link the sorts of every type; built one type per
// object, it links only the ones it uses. The body is algorithm.cpp's.

#include <algorithm>
#include <bit>

_LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_EXPLICIT_ABI_ANNOTATIONS

template <class Comp, class RandomAccessIterator>
void __sort(RandomAccessIterator first, RandomAccessIterator last, Comp) {
  if (first == last) // log(0) is undefined, so don't try computing the depth
    return;

  auto depth_limit = 2 * std::__bit_log2(static_cast<size_t>(last - first));
  std::__introsort<_ClassicAlgPolicy, ranges::less, RandomAccessIterator,
                   __use_branchless_sort<ranges::less, RandomAccessIterator>>(
      first, last, ranges::less{}, depth_limit);
}

template void __sort<__less<TYPE> &, TYPE *>(TYPE *, TYPE *, __less<TYPE> &);

_LIBCPP_END_EXPLICIT_ABI_ANNOTATIONS
_LIBCPP_END_NAMESPACE_STD
