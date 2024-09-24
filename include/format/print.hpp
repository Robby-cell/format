#ifndef FORMAT_PRINT_HPP_
#define FORMAT_PRINT_HPP_

#include <ostream>

#include "format/format.hpp"

namespace fmt {

template <typename... Args>
auto print(std::ostream& os, const FormatString<Args...> fmt_str,
           const Args&... raw_args) -> void {
  ::std::string_view fmt{fmt_str};
  size_t index{0};

  FormatArgs<const Args*...> fmt_args{
      ::std::forward<const Args*>(&raw_args)...};
  MappedArgs<Args...> args{fmt_args};

  while (not fmt.empty()) {
    const auto left{fmt.find_first_of('{')};
    if (left == ::std::string_view::npos) {
      os << fmt;
      return;
    }
    const auto right{fmt.find_first_of('}')};
    if (right == ::std::string_view::npos) {
      _throw_format_error("Missing closing brace");
    }
    const ::std::string_view format_specifier_str{
        fmt.substr(left + 1, right - left - 1)};
    FormatSpecifier specifier{format_specifier_str};
    if (not specifier.has_position_) {
      specifier.position_ = index;
    }
    os << fmt.substr(0, left);
    args.at(specifier.position_)->stream(os, specifier);
    fmt = fmt.substr(right + 1);

    ++index;
  }
}

}  // namespace fmt

#endif  // FORMAT_PRINT_HPP_
