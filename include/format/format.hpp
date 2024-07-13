#ifndef FORMAT_FORMAT_HPP_
#define FORMAT_FORMAT_HPP_

#include <array>
#include <cstring>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace format {

namespace detail {
constexpr inline auto is_alpha(const char c) -> bool {
  return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
}
constexpr inline auto is_digit(const char c) -> bool {
  return c >= '0' and c <= '9';
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
         ::format::_impl_tuple_fold_size<Index + 1, Args...>(t);
}

template <typename... Args>
constexpr auto tuple_fold_size(const ::std::tuple<Args...>& t)
    -> ::std::size_t {
  return ::format::_impl_tuple_fold_size<0, Args...>(t);
}

template <typename... Args>
struct FormatArgs {
  constexpr explicit FormatArgs(Args&&... args)
      : args_{::std::make_tuple<Args...>(::std::forward<Args>(args)...)} {}

  constexpr inline auto estimate_size() const noexcept -> ::std::size_t {
    return ::format::tuple_fold_size<Args...>(args_);
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

}  // namespace format

namespace std {
template <::std::size_t Index, typename... Args>
constexpr auto get(const format::FormatArgs<Args...>& fmt) {
  return ::std::get<Index>(fmt.args_);
}
}  // namespace std

namespace format {

constexpr auto count_placeholders(const ::std::string_view fmt)
    -> ::std::size_t {
  ::std::size_t count{0};

  enum class State { Base, Left } state;

  for (::std::size_t i = 0; i < fmt.length(); ++i) {
    switch (state) {
      case State::Base:
        if (fmt[i] == '{') {
          state = State::Left;
        }
        break;
      case State::Left:
        if (fmt[i] == '}') {
          ++count;
          state = State::Base;
        } else {
          // Invalid state for a placeholder format
          return 0;
        }
        break;
    }
  }

  return count;
}

class FormatSpecifier {
 public:
  static constexpr size_t Hex{1 << 0}, Octal{1 << 1}, Binary{1 << 2},
      Float{1 << 3}, Char{1 << 4}, Pointer{1 << 5};

  constexpr explicit FormatSpecifier(size_t position, size_t specifiers = 0ULL)
      : position_{position}, specifiers_{specifiers} {}

  constexpr explicit FormatSpecifier(const ::std::string_view fmt) {
    parse_specifier(fmt);
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

    if (state == State::End) {
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

class BasicAppendable {
 public:
  constexpr BasicAppendable() = default;
  virtual ~BasicAppendable() = default;
  virtual void append(::std::string& str,
                      const FormatSpecifier& specifier) const = 0;
};

template <typename Type>
class Appendable;

template <>
class Appendable<const char*> : public BasicAppendable {
 public:
  constexpr explicit Appendable(const char* val) : val_(val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    str.append(val_);
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
  void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    str.append(*val_);
  }

 private:
  const ::std::string* val_;
};
template <typename T>
  requires(::std::is_trivial<T>::value)
class Appendable<T> : public BasicAppendable {
 public:
  constexpr explicit Appendable(T val) : val_(val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    str.append(::std::to_string(val_));
  }

 private:
  T val_;
};

template <typename... ArgsType>
constexpr auto estimate_space(const FormatArgs<ArgsType...> args)
    -> ::std::size_t {
  return args.estimate_size();
}

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

class Formatter {
 public:
  constexpr explicit Formatter(const ::std::string_view fmt) : fmt_{fmt} {}

  template <typename... Args>
  inline auto operator()(const FormatArgs<Args...>& args) -> ::std::string {
    ::std::string out{};
    out.reserve(args.estimate_size() + fmt_.length());

    auto mapped_args{map_args<Args...>(args)};
    size_t index{0};

    while (not is_done()) {
      // auto span{next_span()};
      // out.append(span);

      // if (not is_done()) {
      //   mapped_args.at(index++)->append(out);
      // } else {
      //   break;
      // }
      switch (state) {
        case State::Top: {
          if (fmt_.at(0) == '{') {
            state = State::ArgParse;
          } else {
            auto span{next_span()};
            out.append(span);
          }
          break;
        }
        case State::ArgParse: {
          mapped_args.at(index++)->append(out);
          auto end{fmt_.find_first_of('}')};
          if (end == ::std::string_view::npos) {
            throw ::std::runtime_error("Missing closing brace");
          }
          fmt_ = fmt_.substr(end + 1);
          state = State::Top;
          break;
        }
      }
    }

    for (auto& arg : mapped_args) {
      delete arg;
    }

    return out;
  }

  constexpr inline auto next_span() noexcept -> ::std::string_view {
    auto begin{fmt_.find_first_of('{')};

    if (begin == ::std::string_view::npos) {
      return fmt_;
    }
    ::std::string_view next{fmt_.substr(0, begin)};
    fmt_ = fmt_.substr(begin);

    return next;
  }

  constexpr inline auto is_done() const noexcept -> bool {
    return fmt_.empty();
  }

 private:
  ::std::string_view fmt_;
  enum class State { Top, ArgParse } state = State::Top;
};

template <typename... ArgsType>
class FormatString {
 public:
  constexpr explicit FormatString(const ::std::string_view fmt) : fmt_{fmt} {}
  constexpr FormatString() = default;
  constexpr ~FormatString() = default;

  constexpr inline auto get_fmt() const noexcept -> ::std::string_view {
    return fmt_;
  }

  constexpr inline auto operator+=(const ::std::size_t offset) noexcept
      -> FormatString& {
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
  constexpr inline auto front() const noexcept -> const char& {
    return fmt_.front();
  }
  constexpr inline auto back() const noexcept -> const char& {
    return fmt_.back();
  }
  constexpr inline auto at(const ::std::size_t index) const noexcept -> const
      char& {
    return fmt_.at(index);
  }
  constexpr inline auto operator[](const ::std::size_t index) const noexcept
      -> const char& {
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
  constexpr inline auto substr(const ::std::size_t offset,
                               ::std::size_t count = ::std::string_view::npos)
      const noexcept -> ::std::string_view {
    return fmt_.substr(offset, count);
  }

  operator ::std::string_view() const noexcept { return fmt_; }  // NOLINT

 private:
  ::std::string_view fmt_;
};

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
constexpr inline auto _format_impl(FormatString<ArgsType...>& fmt,
                                   MappedArgs<ArgsType...>& args,
                                   ::std::string& out,
                                   const ::size_t index = 0) -> void {
  if (fmt.empty()) {
    return;
  }
  const auto left{fmt.find_first_of('{')};
  if (left == ::std::string_view::npos) {
    out.append(fmt.get_fmt());
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
  fmt.creep(right + 1);

  _format_impl(fmt, args, out, index + 1);
}

template <typename... ArgsType>
constexpr auto format(const ::std::string_view fmt,
                      ArgsType... args_pack) -> ::std::string {
  const FormatArgs<ArgsType...> args{::std::forward<ArgsType>(args_pack)...};

  ::std::string out{};
  out.reserve(args.estimate_size() + fmt.length());

  FormatString<ArgsType...> fmt_string{fmt};
  MappedArgs<ArgsType...> mapped_args{args};

  _format_impl(fmt_string, mapped_args, out);

  return out;
}

}  // namespace format

#endif  // FORMAT_FORMAT_HPP_
