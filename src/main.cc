#include <iostream>

#include "format/detail.hpp"
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
  static void buf_print(std::string& str, [[maybe_unused]] const Foo& val,
                        [[maybe_unused]] const FormatSpecifier& specifier) {
    str.append("Foo");
  }
};
template <>
class Formatter<Point> {
 public:
  static void buf_print(std::string& str, [[maybe_unused]] const Point& val,
                        [[maybe_unused]] const FormatSpecifier& specifier) {
    str.append("(");
    str.append(detail::to_float(val.x));
    str.append(", ");
    str.append(detail::to_float(val.y));
    str.append(")");
  }
};
}  // namespace fmt

#include "format/format.hpp"

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
