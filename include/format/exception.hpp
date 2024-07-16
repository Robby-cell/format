#ifndef FORMAT_EXCEPTION_HPP_
#define FORMAT_EXCEPTION_HPP_

#include <stdexcept>
namespace fmt {
class FormatError : public ::std::runtime_error {
  using ::std::runtime_error::runtime_error;
};
[[noreturn]] inline void _throw_format_error(const char* const what) {
  throw FormatError(what);
}
}  // namespace fmt

#endif  // FORMAT_EXCEPTION_HPP_
