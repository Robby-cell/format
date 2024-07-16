#ifndef FORMAT_DETAIL_HPP_
#define FORMAT_DETAIL_HPP_

#include <bitset>
#include <string>

namespace fmt::detail {

constexpr inline auto is_alpha(const char c) -> bool {
  return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
}
constexpr inline auto is_digit(const char c) -> bool {
  return c >= '0' and c <= '9';
}

inline constexpr ::std::string_view const HexDigits{"0123456789ABCDEF"};
template <typename Type>
inline auto to_hex(Type n, [[maybe_unused]] ::std::size_t hex_len = 0ULL)
    -> ::std::string {
  constexpr auto Length{sizeof(Type) << 1};
  std::string result{};
  result.resize(Length);

  for (size_t current = 0, j = (Length - 1) * 4; current < Length;
       ++current, j -= 4) {
    result[current] = HexDigits[(n >> j) & 0x0f];
  }

  return result;
}

inline constexpr ::std::string_view const OctalDigits{"01234567"};
template <typename Type>
inline auto to_octal(Type n, ::std::size_t len = 0ULL) -> ::std::string {
  std::string result(len, '0');
  for (size_t i = 0, j = (len - 1) * 4; i < len; ++i, j -= 4) {
    result[i] = HexDigits[(n >> j) & 0x0f];
  }
  return result;
}

template <typename Type>
inline auto to_binary(Type n, [[maybe_unused]] ::std::size_t len = 0ULL)
    -> ::std::string {
  auto bin{::std::bitset<sizeof(Type) * 8>(n).to_string()};

  ::std::size_t left{bin.find_first_of('1')};
  if (left == ::std::string::npos) {
    return bin;
  } else if (len < bin.length() - left) {
    return bin.substr(left);
  }

  return bin;
}

template <typename Type>
inline auto to_decimal(Type n, [[maybe_unused]] ::std::size_t len = 0ULL)
    -> ::std::string {
  return std::to_string(n);
}

template <typename Type>
inline auto to_float(Type n, [[maybe_unused]] ::std::size_t len = 0ULL)
    -> ::std::string {
  return std::to_string(n);
}

}  // namespace fmt::detail

#endif  // FORMAT_DETAIL_HPP_
