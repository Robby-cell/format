#ifndef FORMAT_FORMAT_HPP_
#define FORMAT_FORMAT_HPP_

#include <array>
#include <concepts>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

namespace fmt {

// NOLINTBEGIN
using u8 = ::std::uint8_t;
using u16 = ::std::uint16_t;
using u32 = ::std::uint32_t;
using u64 = ::std::uint64_t;
using usize = ::std::size_t;

using i8 = ::std::int8_t;
using i16 = ::std::int16_t;
using i32 = ::std::int32_t;
using i64 = ::std::int64_t;
using isize = ::std::ptrdiff_t;

using f32 = float;
using f64 = double;
// NOLINTEND

namespace detail {
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
}  // namespace detail

class FormatError : public ::std::runtime_error {
  using ::std::runtime_error::runtime_error;
};
[[noreturn]] inline void _throw_format_error(const char* const what) {
  throw FormatError(what);
}

template <typename, typename... Args>
struct CountParameterPack {
  static constexpr ::std::size_t Value{1 + CountParameterPack<Args...>::Value};
};

template <typename Type>
struct CountParameterPack<Type> {
  static constexpr ::std::size_t Value{1};
};

template <typename... Args>
static constexpr inline auto parameter_pack_arity() {
  return CountParameterPack<Args...>::Value;
}

template <typename T>
struct FormatArgsEstimate {
  static constexpr inline auto size([[maybe_unused]] const T& unused)
      -> ::std::size_t {
    return 8;
  }
};
template <>
struct FormatArgsEstimate<::std::string> {
  static constexpr inline auto size(const ::std::string& str) -> ::std::size_t {
    return str.length();
  }
};
template <>
struct FormatArgsEstimate<const char*> {
  static constexpr inline auto size([[maybe_unused]] const char* str)
      -> ::std::size_t {
    return 32UL;
  }
};

template <typename Type>
constexpr inline auto estimate_size(const Type& t) -> ::std::size_t {
  return FormatArgsEstimate<Type>::size(t);
}

template <size_t Index = 0, typename... Args>
constexpr auto _impl_tuple_fold_size(const ::std::tuple<Args...>& t)
    -> ::std::size_t;

template <size_t Index = 0, typename... Args>
  requires(Index >= parameter_pack_arity<Args...>())
constexpr auto _impl_tuple_fold_size(
    [[maybe_unused]] const ::std::tuple<Args...>& t) -> ::std::size_t {
  return 0;
}
template <size_t Index = 0, typename... Args>
  requires(Index < parameter_pack_arity<Args...>())
constexpr auto _impl_tuple_fold_size(const ::std::tuple<Args...>& t)
    -> ::std::size_t {
  return estimate_size(::std::get<Index>(t)) +
         ::fmt::_impl_tuple_fold_size<Index + 1, Args...>(t);
}

template <typename... Args>
constexpr auto tuple_fold_size(const ::std::tuple<Args...>& t)
    -> ::std::size_t {
  return ::fmt::_impl_tuple_fold_size<0, Args...>(t);
}

template <typename... Args>
struct FormatArgs {
  constexpr explicit FormatArgs(Args&&... args)
      : args_{::std::make_tuple<Args...>(::std::forward<Args>(args)...)} {}

  constexpr inline auto estimate_size() const noexcept -> ::std::size_t {
    return ::fmt::tuple_fold_size<Args...>(args_);
  }

  template <size_t Index>
  constexpr auto get() const noexcept -> const
      decltype(::std::get<Index>(::std::declval<::std::tuple<Args...>>()))& {
    return ::std::get<Index>(args_);
  }

  template <size_t Index>
  constexpr auto get() noexcept
      -> decltype(::std::get<Index>(::std::declval<::std::tuple<Args...>>())) {
    return ::std::get<Index>(args_);
  }

 private:
  template <::std::size_t, typename... GetArgs>
  friend constexpr auto ::std::get(const FormatArgs<GetArgs...>&);

  std::tuple<Args...> args_;
};

}  // namespace fmt

namespace std {
template <::std::size_t Index, typename... Args>
constexpr auto get(const ::fmt::FormatArgs<Args...>& fmt) {
  return ::std::get<Index>(fmt.args_);
}
}  // namespace std

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

// TODO : APPENDABLE

template <typename... Types>
struct TypeList;

using UnsignedIntType = TypeList<u8, u16, u32, u64, usize>;
using UnsignedIntNoCharType = TypeList<u16, u32, u64, usize>;
using SignedIntType = TypeList<i8, i16, i32, i64, isize>;
using SignedIntNoCharType = TypeList<i16, i32, i64, isize>;
using FloatType = TypeList<f32, f64>;

template <typename Type, typename...>
constexpr inline bool IsAnyOfImpl = false;

template <typename Type, typename... Args>
constexpr inline bool IsAnyOfImpl<Type, TypeList<Args...>> =
    (::std::is_same<Type, Args>::value or ...);  // NOLINT

template <typename Type, typename List>
concept IsAnyOf = IsAnyOfImpl<Type, List>;

template <typename Type>
concept IsUnsignedInteger =
    requires(Type) { requires(IsAnyOf<Type, UnsignedIntType>); };
template <typename Type>
concept IsUnsignedIntegerNoChar =
    requires(Type) { requires(IsAnyOf<Type, UnsignedIntNoCharType>); };
template <typename Type>
concept IsSignedInteger =
    requires(Type) { requires(IsAnyOf<Type, SignedIntType>); };
template <typename Type>
concept IsSignedIntegerNoChar =
    requires(Type) { requires(IsAnyOf<Type, SignedIntNoCharType>); };

template <typename Type>
concept IsInteger = IsSignedInteger<Type> or IsUnsignedInteger<Type>;
template <typename Type>
concept IsIntegerNoChar =
    IsSignedIntegerNoChar<Type> or IsUnsignedIntegerNoChar<Type>;

template <typename Type>
concept IsFloat = requires(Type) { requires(IsAnyOf<Type, FloatType>); };

class BasicAppendable {
 public:
  constexpr BasicAppendable() = default;
  virtual ~BasicAppendable() = default;
  virtual constexpr void append(::std::string& str,
                                const FormatSpecifier& specifier) const = 0;
};

template <typename Type>
struct Print;
template <>
struct Print<::std::string_view> {
  static constexpr auto buf_print(::std::string& str,
                                  const ::std::string_view val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    str.append(val);
  }
};
template <>
struct Print<const char*> {
  static constexpr auto buf_print(::std::string& str, const char* const val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    str.append(val);
  }
};
template <>
struct Print<::std::string> {
  static constexpr auto buf_print(::std::string& str, const ::std::string& val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    str.append(val);
  }
};
template <IsIntegerNoChar Type>
struct Print<Type> {
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
struct Print<Type> {
  static constexpr auto buf_print(::std::string& str, Type val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    str.append(detail::to_float(val));
  }
};
template <>
struct Print<char> {
  static constexpr auto buf_print(::std::string& str, const char val,
                                  const FormatSpecifier& specifiers) -> void {
    (void)specifiers;
    if (specifiers.is_char()) {
      str.push_back(val);
    } else {
      Print<int>::buf_print(str, val, specifiers);
    }
  }
};
template <typename Type>
constexpr inline auto buf_print(::std::string& str, const Type& val,
                                const FormatSpecifier& specifiers) -> void {
  Print<Type>::buf_print(str, val, specifiers);
}

template <typename Type>
class Appendable : public BasicAppendable {
 public:
  constexpr explicit Appendable(const Type& val) : val_(&val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  constexpr void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    // str.append(val_);
    buf_print(str, val_, specifier);
  }

 private:
  const Type* val_;
};

template <>
class Appendable<const char*> : public BasicAppendable {
 public:
  constexpr explicit Appendable(const char* val) : val_(val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  constexpr void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    // str.append(val_);
    buf_print(str, val_, specifier);
  }

 private:
  const char* val_;
};
template <>
class Appendable<::std::string> : public BasicAppendable {
 public:
  constexpr explicit Appendable(const ::std::string& val) : val_(&val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  constexpr void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    buf_print(str, *val_, specifier);
  }

 private:
  const ::std::string* val_;
};

template <IsIntegerNoChar Type>
class Appendable<Type> : public BasicAppendable {
 public:
  constexpr explicit Appendable(Type val) : val_(val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  constexpr void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    if (specifier.specifiers_ & FormatSpecifier::Hex) {
      if (specifier.has_size_) {
        str.append(detail::to_hex(val_, specifier.size_));
      } else {
        str.append(detail::to_hex(val_));
      }
    } else {
      str.append(::std::to_string(val_));
    }
  }

 private:
  Type val_;
};

template <IsFloat Type>
class Appendable<Type> : public BasicAppendable {
 public:
  constexpr explicit Appendable(Type val) : val_(val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  constexpr void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    buf_print(str, val_, specifier);
  }

 private:
  Type val_;
};

template <typename Type>
constexpr inline auto make_appendable(const Type& val) -> BasicAppendable* {
  return new (::std::nothrow) Appendable<Type>(val);
}

template <size_t Index = 0, typename... ArgsType>
  requires(Index >= parameter_pack_arity<ArgsType...>())
constexpr inline auto array_fill(
    std::array<BasicAppendable*, parameter_pack_arity<ArgsType...>()>& arr,
    const FormatArgs<ArgsType...>& args) noexcept -> void {}

template <size_t Index = 0, typename... ArgsType>
  requires(Index < parameter_pack_arity<ArgsType...>())
constexpr inline auto array_fill(
    std::array<BasicAppendable*, parameter_pack_arity<ArgsType...>()>& arr,
    const FormatArgs<ArgsType...>& args) noexcept -> void {
  arr[Index] = make_appendable(::std::get<Index>(args));
  array_fill<Index + 1, ArgsType...>(arr, args);
}

template <typename... ArgsType>
constexpr inline auto map_args(
    const FormatArgs<ArgsType...>& format_args) noexcept
    -> ::std::array<BasicAppendable*, parameter_pack_arity<ArgsType...>()> {
  ::std::array<BasicAppendable*, parameter_pack_arity<ArgsType...>()> args{};

  array_fill<0>(args, format_args);

  return args;
}

template <typename MyChar, typename... ArgsType>
class FormatStringImpl {
  static constexpr auto Arity = parameter_pack_arity<ArgsType...>();
  using ArgType = ::std::array<FormatSpecifier, Arity * 3>;

 public:
  template <class Type>
    requires ::std::convertible_to<const Type&,
                                   ::std::basic_string_view<MyChar>>
  consteval FormatStringImpl(const Type& fmt) : fmt_{fmt} {  // NOLINT
    verify_arg_count();
  }
  constexpr FormatStringImpl() = default;
  constexpr ~FormatStringImpl() = default;

  constexpr inline auto get_fmt() const noexcept
      -> ::std::basic_string_view<MyChar> {
    return fmt_;
  }

  constexpr inline auto operator+=(const ::std::size_t offset) noexcept
      -> FormatStringImpl& {
    fmt_ = fmt_.substr(offset);
    return *this;
  }

  constexpr inline auto creep(const ::std::size_t offset) noexcept -> void {
    fmt_ = fmt_.substr(offset);
  }

  constexpr inline auto length() const noexcept -> ::std::size_t {
    return fmt_.length();
  }
  constexpr inline auto empty() const noexcept -> bool { return fmt_.empty(); }
  constexpr inline auto front() const noexcept -> const MyChar& {
    return fmt_.front();
  }
  constexpr inline auto back() const noexcept -> const MyChar& {
    return fmt_.back();
  }
  constexpr inline auto at(const ::std::size_t index) const noexcept
      -> const MyChar& {
    return fmt_.at(index);
  }
  constexpr inline auto operator[](const ::std::size_t index) const noexcept
      -> const MyChar& {
    return fmt_[index];
  }
  template <typename Type>
  constexpr inline auto find_first_of(const Type c) const noexcept
      -> ::std::size_t {
    return fmt_.find_first_of(c);
  }
  template <typename Type>
  constexpr inline auto find_last_of(const Type c) const noexcept
      -> ::std::size_t {
    return fmt_.find_last_of(c);
  }
  constexpr inline auto substr(
      const ::std::size_t offset,
      ::std::size_t count = ::std::basic_string_view<MyChar>::npos)
      const noexcept -> ::std::basic_string_view<MyChar> {
    return fmt_.substr(offset, count);
  }

  // NOLINTBEGIN
  operator ::std::basic_string_view<MyChar>() const noexcept { return fmt_; }
  // NOLINTEND

  constexpr inline auto verify_arg_count() -> void {
    ArgType args{};
    [[maybe_unused]] auto count{count_format_args(args)};

    auto max{::std::accumulate(
        args.begin(), args.end(), 0ULL,
        [](auto a, auto b) { return a > b.position_ ? a : b.position_; })};

    if (count < Arity or max >= Arity) {
      _throw_format_error("Too few arguments");
    }

    for (const auto& item : args) {
      if (item.position_ > max or item.position_ >= Arity) {
        _throw_format_error("Not enough arguments");
      }
    }
    for (::std::size_t i = 0; i < max; ++i) {
      bool found{false};
      for (const auto& item : args) {
        if (item.position_ == i) {
          found = true;
          break;
        }
      }
      if (not found) {
        _throw_format_error("All positions must be used.");
      }
    }
  }

  constexpr inline auto count_format_args(ArgType& args) -> ::std::size_t {
    constexpr auto npos{::std::basic_string_view<MyChar>::npos};  // NOLINT

    const MyChar* current{fmt_.data()};
    const MyChar* const end{fmt_.data() + fmt_.length()};

    ::std::size_t count{0};

    while (current not_eq end) {
      auto left{
          ::std::basic_string_view<MyChar>{current, end}.find_first_of('{')};
      if (left not_eq npos) {
        auto right{
            ::std::basic_string_view<MyChar>{current, end}.find_first_of('}')};
        if (right == npos) {
          _throw_format_error("Missing closing brace");
        }
        ::std::basic_string_view<MyChar> format_specifier_str{
            current + left + 1, right - left - 1};
        FormatSpecifier specifier{format_specifier_str};
        if (not specifier.has_position_) {
          specifier.position_ = count;
          specifier.has_position_ = true;
        }
        args.at(count++) = specifier;

        current += right + 1;
      } else {
        current = end;
      }
    }
    return count;
  }

 private:
  ::std::basic_string_view<MyChar> fmt_;
};
template <typename... ArgsType>
using FormatString =
    FormatStringImpl<char, ::std::type_identity_t<ArgsType>...>;

template <typename... ArgsType>
class MappedArgs {
 public:
  static constexpr auto Arity = parameter_pack_arity<ArgsType...>();
  constexpr explicit MappedArgs(const FormatArgs<ArgsType...>& args)
      : args_{map_args<ArgsType...>(args)} {}
  constexpr MappedArgs() = default;
  constexpr ~MappedArgs() {
    for (auto& arg : args_) {
      delete arg;
    }
  }

  constexpr inline auto at(const ::std::size_t index) const noexcept
      -> const BasicAppendable* const& {
    return args_.at(index);
  }
  constexpr inline auto at(const ::std::size_t index) noexcept
      -> BasicAppendable*& {
    return args_.at(index);
  }

 private:
  ::std::array<BasicAppendable*, Arity> args_;
};

template <typename... ArgsType>
constexpr inline auto _format_impl(FormatString<ArgsType...>& fmt_str,
                                   MappedArgs<ArgsType...>& args,
                                   ::std::string& out,
                                   ::size_t index = 0) -> void {
  ::std::string_view fmt{fmt_str};
  while (not fmt.empty()) {
    const auto left{fmt.find_first_of('{')};
    if (left == ::std::string_view::npos) {
      out.append(fmt);
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
    out.append(fmt.substr(0, left));
    args.at(specifier.position_)->append(out, specifier);
    // fmt += right + 1;
    fmt = fmt.substr(right + 1);

    ++index;
  }

  // _format_impl(fmt, args, out, index + 1);
}

template <typename... ArgsType>
[[nodiscard]] constexpr auto format(FormatString<ArgsType...> fmt,
                                    ArgsType&&... args_pack) -> ::std::string {
  const FormatArgs<ArgsType...> args{::std::forward<ArgsType>(args_pack)...};

  ::std::string out{};
  out.reserve(args.estimate_size() + fmt.length());

  MappedArgs<ArgsType...> mapped_args{args};

  _format_impl(fmt, mapped_args, out);

  return out;
}

}  // namespace fmt

#endif  // FORMAT_FORMAT_HPP_
