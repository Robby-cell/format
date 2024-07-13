#include <iostream>

#include "format/format.hpp"

struct Foo {
  explicit Foo(int x) {}
};

using fmt::format;

auto main() -> int try {
  const auto s = format("Hello {}", 32);
  auto t = format("{}", 42);
  std::cout << s << '\n';
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
