#include <iostream>

#include "format/format.hpp"

struct Foo {
  explicit Foo(int x) {}
};

auto main() -> int try {
  auto s = format::format("Hello world {1} {0}", 42, "another one");
  std::cout << s << '\n';

  return 0;
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
