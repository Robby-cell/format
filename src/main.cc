#include <iostream>

#include "format/formatter.hpp"

struct Foo {
  explicit Foo(int x) {}
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
}  // namespace fmt

#include "format/format.hpp"

auto main() -> int try {
  Foo foo{42};
  const char* foo_ptr{"foo"};
  const auto s = fmt::format("Hello, {}!", "world");
  std::cout << s << '\n';
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
