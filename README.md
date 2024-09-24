# Format

## What is it for?

This is a hobby project to play around with templates in C++. Currently have only used it with MSVC.
The funcionality it provides should be similar to `std::print`/`std::format`, in modern versions of C++.
Still working on format specifiers, (`{:4x}`, take 4 chars, and display as hexadecimal etc)

## How to use it?

- `fmt::format`
```cpp
#include "format/format.hpp"
auto main() -> int {
  auto my_string{ fmt::format("Hello, {}!", "World") }; // my_string is "Hello, World!"
}
```

- `fmt::print`
```cpp
#include "format/print.hpp"
#include <iostream> // std::cout, fmt::print needs a stream to print to.
auto main() -> int {
  fmt::print(std::cout, "{1}, {0}!\n", "World", "Hello"); // prints "Hello, World!\n"
}
```

- to print/format custom types
```cpp
#include "format/format.hpp"
struct Foo {};
namespace fmt {
// Create specialization for the formatter, for when Foo is used.
template <>
class Formatter<Foo> {
 public:
  static void buf_print(std::string& str, [[maybe_unused]] const Foo& val,
                        [[maybe_unused]] const FormatSpecifier& specifier) {
    str.append("Foo");
  }
};
}
auto main() -> int {
  Foo foo;
  auto str = fmt::format("{}", foo); // str is "Foo"
}
```
