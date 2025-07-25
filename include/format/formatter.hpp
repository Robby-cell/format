#ifndef FORMAT_FORMATTER_HPP_
#define FORMAT_FORMATTER_HPP_

#include <string>
#include <string_view>

#include "format/concept.hpp"
#include "format/specifier.hpp"

namespace fmt {

template <typename Type>
struct Formatter;

template <>
struct Formatter<::std::string_view> {
  template <class Os>
  static constexpr auto buf_print(Os& os, const ::std::string_view val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    os << val;
  }
};
template <>
struct Formatter<const char*> {
  template <class Os>
  static constexpr auto buf_print(Os& os, const char* const val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    os << val;
  }
};
template <>
struct Formatter<::std::string> {
  template <class Os>
  static constexpr auto buf_print(Os& os, const ::std::string& val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    os << val;
  }
};
template <IsIntegerNoChar Type>
struct Formatter<Type> {
  template <class Os>
  static constexpr auto buf_print(Os& os, Type val,
                                  const FormatSpecifier& specifiers) -> void {
    auto size{specifiers.has_size_ ? specifiers.size_ : 0};
    if (specifiers.is_hex()) {
      os << detail::to_hex(val, size);
    } else if (specifiers.is_octal()) {
      os << detail::to_octal(val, size);
    } else if (specifiers.is_binary()) {
      os << detail::to_binary(val, size);
    } else {
      os << detail::to_decimal(val, size);
    }
  }
};
template <IsFloat Type>
struct Formatter<Type> {
  template <class Os>
  static constexpr auto buf_print(Os& os, Type val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    os << detail::to_float(val);
  }
};
template <>
struct Formatter<char> {
  template <class Os>
  static constexpr auto buf_print(Os& os, const char val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    if (specifiers.is_char()) {
      os << val;
    } else {
      os << static_cast<int>(val);
    }
  }
};

}  // namespace fmt

#endif  // FORMAT_FORMATTER_HPP_
