#ifndef FORMAT_DETAIL_HPP_
#define FORMAT_DETAIL_HPP_

#include <string>

namespace fmt::detail {

constexpr inline auto is_alpha(const char c) -> bool {
  return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
}
constexpr inline auto is_digit(const char c) -> bool {
  return c >= '0' and c <= '9';
}

static constexpr const char* const HexDigits{"0123456789ABCDEF"};
template <typename Type>
auto to_hex(Type n, size_t hex_len = sizeof(Type) << 1) -> ::std::string {
  std::string result(hex_len, '0');
  for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
    result[i] = HexDigits[(n >> j) & 0x0f];
  }
  return result;
}
template <typename Type>
auto to_octal(Type n, size_t hex_len = sizeof(Type) << 1) -> ::std::string {
  std::string result(hex_len, '0');
  for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
    result[i] = HexDigits[(n >> j) & 0x0f];
  }
  return result;
}
template <typename Type>
auto to_binary(Type n, size_t hex_len = sizeof(Type) << 1) -> ::std::string {
  std::string result(hex_len, '0');
  for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
    result[i] = HexDigits[(n >> j) & 0x0f];
  }
  return result;
}
template <typename Type>
auto to_decimal(Type n, size_t hex_len = sizeof(Type) << 1) -> ::std::string {
  std::string result(hex_len, '0');
  for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
    result[i] = HexDigits[(n >> j) & 0x0f];
  }
  return result;
}
template <typename Type>
auto to_float(Type n, size_t hex_len = sizeof(Type) << 1) -> ::std::string {
  std::string result(hex_len, '0');
  for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
    result[i] = HexDigits[(n >> j) & 0x0f];
  }
  return result;
}

}  // namespace fmt::detail

#endif  // FORMAT_DETAIL_HPP_
