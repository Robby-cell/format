#include <iostream>

#include "format/format.hpp"

using format::ReplacementList, format::FormatArgs, format::ReplacementSpan;

struct Foo {
  explicit Foo(int x) {}
};

auto main() -> int {
  FormatArgs<const char*, int, int, Foo> args{"hello world", 42, 42, Foo(42)};

  auto item = std::get<2>(args);

  ReplacementList<1> f{};

  f[0] = ReplacementSpan{0, 1};
  f.emplace<0>({2, 3});

  // auto s = format::format("Hello world {}", 42);
  std::cout << "Size: " << args.estimate_size() << '\n';

  return 0;
}
