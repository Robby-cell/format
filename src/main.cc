#include <iostream>

#include "format/detail.hpp"
#include "format/format.hpp"
#include "format/formatter.hpp"
#include "format/print.hpp"

struct Foo {
  explicit Foo(int x) {}
};
struct Point {
  float x;
  float y;
};
namespace fmt {
template <>
class Formatter<Foo> {
 public:
  template <class Os>
  static void buf_print(Os& os, [[maybe_unused]] const Foo& val,
                        [[maybe_unused]] const FormatSpecifier& specifier) {
    os << "Foo";
  }
};
template <>
class Formatter<Point> {
 public:
  template <class Os>
  static void buf_print(Os& os, [[maybe_unused]] const Point& val,
                        [[maybe_unused]] const FormatSpecifier& specifier) {
    os << '(';
    os << detail::to_float(val.x);
    os << ", ";
    os << detail::to_float(val.y);
    os << ')';
  }
};
}  // namespace fmt

auto main() -> int try {
  Foo foo{42};
  const char* foo_ptr{"foo"};
  const auto s = fmt::format("Hello, {}! Location = {}. {:4x}", foo,
                             Point{1.0F, 2.0F}, 260);
  std::cout << s << '\n';

  fmt::print(std::cout, "Hello, {}! Location = {}. {:4x}", foo,
             Point{1.0F, 2.0F}, 260);
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
