#ifndef FORMAT_SPECIFIER_HPP_
#define FORMAT_SPECIFIER_HPP_

#include <cstddef>
#include <string_view>

#include "format/detail.hpp"
#include "format/exception.hpp"

namespace fmt {
class FormatSpecifier {
 public:
  static constexpr size_t Hex{1 << 0}, Octal{1 << 1}, Binary{1 << 2},
      Float{1 << 3}, Char{1 << 4}, Pointer{1 << 5};

  constexpr explicit FormatSpecifier(size_t position, size_t specifiers = 0ULL)
      : position_{position}, specifiers_{specifiers} {}

  constexpr explicit FormatSpecifier(const ::std::string_view fmt) {
    parse_specifier(fmt);
  }

  constexpr inline auto is_hex() const noexcept -> bool {
    return specifiers_ & FormatSpecifier::Hex;
  }
  constexpr inline auto is_octal() const noexcept -> bool {
    return specifiers_ & FormatSpecifier::Octal;
  }
  constexpr inline auto is_binary() const noexcept -> bool {
    return specifiers_ & FormatSpecifier::Binary;
  }
  constexpr inline auto is_float() const noexcept -> bool {
    return specifiers_ & FormatSpecifier::Float;
  }
  constexpr inline auto is_char() const noexcept -> bool {
    return specifiers_ & FormatSpecifier::Char;
  }
  constexpr inline auto is_pointer() const noexcept -> bool {
    return specifiers_ & FormatSpecifier::Pointer;
  }

  /// @brief Default constructor so an array can be created without needing to
  /// initialize all the specifiers
  constexpr FormatSpecifier() = default;
  constexpr ~FormatSpecifier() = default;

 private:
  constexpr inline auto parse_specifier(const ::std::string_view fmt) -> void {
    if (fmt.length() == 0) {
      return;
    }

    enum class State {
      Begin,
      Position,
      Fill,
      SizeBegin,
      Size,
      End,
    } state{State::Position};

    const char* current{fmt.data()};
    const char* const end{fmt.data() + fmt.length()};

    const char* size_begin{nullptr};
    const char* size_end{nullptr};
    const char* position_end{nullptr};
    char fill_intermediate{0};

    while (current < end) {
      switch (state) {
        case State::Begin: {
          if (*current == ':') {
            state = State::Fill;
          } else {
            state = State::Position;
          }
        }
        case State::Position: {
          if (*current == ':') {
            position_end = current;
            state = State::Fill;
          } else if (not detail::is_digit(*current)) {
            _throw_format_error("Invalid character in the positional argument");
          }
          break;
        }
        case State::Fill: {
          if (detail::is_alpha(*current)) {
            fill_intermediate = *current;
            state = State::SizeBegin;
          } else if (*current == '0') {
            fill_intermediate = *current;
            state = State::SizeBegin;
          } else if (detail::is_digit(*current)) {
            size_begin = current;
            state = State::Size;
          } else {
            _throw_format_error("Invalid character after ':'");
          }
          break;
        }
        case State::SizeBegin: {
          size_begin = current;
          state = State::Size;
          break;
        }
        case State::Size: {
          if (not detail::is_digit(*current)) {
            size_end = current;
            state = State::End;
          }
          break;
        }
        case State::End: {
          throw ::std::runtime_error(
              "Unexpected additional characters found in format specifier");
        }
      }
      ++current;
    }

    if (fmt.front() not_eq ':') {
      position_ = to_number(fmt.data(), position_end ? position_end : end);
      has_position_ = true;
    }

    if (size_begin) {
      size_ = to_number(size_begin, size_end ? size_end : current);
      has_size_ = true;
    }

    if (state not_eq State::SizeBegin) {
      fill_ = fill_intermediate;
    }

    if (state == State::End or state == State::SizeBegin) {
      char const layout{fmt.back()};
      switch (layout) {
        default: {
          _throw_format_error("Invalid layout specifier");
        }
        case 'X':
        case 'x': {
          specifiers_ |= FormatSpecifier::Hex;
          break;
        }
        case 'O':
        case 'o': {
          specifiers_ |= FormatSpecifier::Octal;
          break;
        }
        case 'B':
        case 'b': {
          specifiers_ |= FormatSpecifier::Binary;
          break;
        }
        case 'F':
        case 'f': {
          specifiers_ |= FormatSpecifier::Float;
          break;
        }
        case 'C':
        case 'c': {
          specifiers_ |= FormatSpecifier::Char;
          break;
        }
        case 'P':
        case 'p': {
          specifiers_ |= FormatSpecifier::Pointer;
          break;
        }
      }
    }
  }
  static constexpr inline auto to_number(const char* ptr, const char* const end)
      -> ::std::size_t {
    ::std::size_t value{0};
    while (ptr not_eq end) {
      if (*ptr < '0' or *ptr > '9') {
        _throw_format_error("Invalid character in number");
      }
      value *= 10;
      value += *ptr - '0';
      ++ptr;
    }
    return value;
  }

 public:
  ::std::size_t specifiers_{0};
  ::std::size_t position_{0};
  ::std::size_t size_{0};
  bool has_position_{false};
  bool has_size_{false};
  char fill_{' '};
};
}  // namespace fmt

#endif  // FORMAT_SPECIFIER_HPP_
