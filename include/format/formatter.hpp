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
  static constexpr auto buf_print(::std::string& str,
                                  const ::std::string_view val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    str.append(val);
  }
};
template <>
struct Formatter<const char*> {
  static constexpr auto buf_print(::std::string& str, const char* const val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    str.append(val);
  }
};
template <>
struct Formatter<::std::string> {
  static constexpr auto buf_print(::std::string& str, const ::std::string& val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    str.append(val);
  }
};
template <IsIntegerNoChar Type>
struct Formatter<Type> {
  static constexpr auto buf_print(::std::string& str, Type val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    if (specifiers.is_hex()) {
      str.append(detail::to_hex(val));
    } else if (specifiers.is_octal()) {
      str.append(detail::to_octal(val));
    } else if (specifiers.is_binary()) {
      str.append(detail::to_binary(val));
    } else {
      str.append(detail::to_decimal(val));
    }
  }
};
template <IsFloat Type>
struct Formatter<Type> {
  static constexpr auto buf_print(::std::string& str, Type val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    str.append(detail::to_float(val));
  }
};
template <>
struct Formatter<char> {
  static constexpr auto buf_print(::std::string& str, const char val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    if (specifiers.is_char()) {
      str.push_back(val);
    } else {
      Formatter<int>::buf_print(str, val, specifiers);
    }
  }
};

}  // namespace fmt

#endif  // FORMAT_FORMATTER_HPP_
