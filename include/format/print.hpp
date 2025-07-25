#ifndef FORMAT_PRINT_HPP_
#define FORMAT_PRINT_HPP_

#include <ostream>

#include "format/format.hpp"

namespace fmt {

template <class Char, typename... Args>
auto print(std::basic_ostream<Char>& os, const FormatString<Args...> fmt_str,
           const Args&... raw_args) -> void {
  MappedArgs<Args...> mapped_args{raw_args...};

  _format_impl(fmt_str, mapped_args, os);
}

}  // namespace fmt

#endif  // FORMAT_PRINT_HPP_
