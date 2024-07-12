#include <iostream>

#include "format/format.hpp"

struct Foo {
  explicit Foo(int x) {}
};

constexpr inline auto FmtStr{format::FormatString<int>("Hello world {0:010x}")};

auto main() -> int try {
  auto s =
      format::format("Hello world {}, and another {} {}", 42, "word", 34.22);
  std::cout << s << '\n';

  const auto& specifiers{FmtStr.get_arg_specifiers()};

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
