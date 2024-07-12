#include <iostream>

#include "format/format.hpp"

struct Foo {
  explicit Foo(int x) {}
};

auto main() -> int try {
  const auto fmt_str{format::FormatString<int, char, int, const char*>(
      "Hello world {} {} {} {}")};

  auto s =
      format::format("Hello world {}, and another {} {}", 42, "word", 34.22);
  std::cout << s << '\n';

  const auto& specifiers{fmt_str.get_arg_specifiers()};

  for (const auto& specifier : specifiers) {
    ::std::cerr << "Specifiers: " << specifier.specifiers_
                << "\nFill: " << specifier.fill_
                << "\nHas position: " << specifier.has_position_
                << "\nPosition: " << specifier.position_
                << "\nSize: " << specifier.size_
                << "\nHas size: " << specifier.has_size_ << "\n";
  }

  return 0;
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
