#include <experimental/ranges/algorithm>
#include <experimental/ranges/iterator>
#include <type_traits>
#include "../simple_test.hpp"

namespace ranges = std::experimental::ranges;

struct moveonly {
  moveonly(int i = 0) noexcept
  : i_{i}
  {}
  moveonly(moveonly&& that) noexcept
  : i_{ranges::exchange(that.i_, 0)}
  {}
  moveonly& operator=(moveonly&& that) & noexcept {
    i_ = ranges::exchange(that.i_, 0);
    return *this;
  }

  int i_ = 0;
};

inline bool operator==(const moveonly& x, const moveonly& y) { return x.i_ == y.i_; }
inline bool operator!=(const moveonly& x, const moveonly& y) { return !(x == y); }
inline bool operator<(const moveonly& x, const moveonly& y) { return x.i_ < y.i_; }
inline bool operator>(const moveonly& x, const moveonly& y) { return y < x; }
inline bool operator<=(const moveonly& x, const moveonly& y) { return !(y < x); }
inline bool operator>=(const moveonly& x, const moveonly& y) { return !(x < y); }

int main() {
  {
    moveonly source[4]{1,2,3,4}; 
    moveonly target[4]{};
    ranges::copy(source, ranges::move_into(target));
    CHECK(ranges::equal({0,0,0,0}, source, ranges::equal_to<>{},
                        ranges::identity{}, &moveonly::i_));
    CHECK(ranges::equal({1,2,3,4}, target, ranges::equal_to<>{},
                        ranges::identity{}, &moveonly::i_));
  }
  {
    moveonly source1[4]{0,2,4,6};
    moveonly source2[4]{1,3,5,7};
    moveonly target[8]{};
    ranges::merge(source1, source2, ranges::move_into(target));
    CHECK(ranges::equal({0,0,0,0}, source1, ranges::equal_to<>{},
                        ranges::identity{}, &moveonly::i_));
    CHECK(ranges::equal({0,0,0,0}, source2, ranges::equal_to<>{},
                        ranges::identity{}, &moveonly::i_));
    CHECK(ranges::equal({0,1,2,3,4,5,6,7}, target, ranges::equal_to<>{},
                        ranges::identity{}, &moveonly::i_));
  }
  {
    moveonly source[4]{1,2,3,4};
    moveonly target[4]{};
    ranges::rotate_copy(source, source + 2, ranges::move_into(target));
    CHECK(ranges::equal({0,0,0,0}, source, ranges::equal_to<>{},
                        ranges::identity{}, &moveonly::i_));
    CHECK(ranges::equal({3,4,1,2}, target, ranges::equal_to<>{},
                        ranges::identity{}, &moveonly::i_));
  }

  return ::test_result();
}
