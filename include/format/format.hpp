#ifndef FORMAT_FORMAT_HPP_
#define FORMAT_FORMAT_HPP_

#include <array>
#include <cstring>
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
    enum class State {
      Position,
      Fill,
      SizeBegin,
      Size,
      End
    } state{State::Position};

    const char* current{fmt.data()};
    const char* const end{fmt.data() + fmt.length()};

    const char* size_begin{nullptr};
    const char* size_end{nullptr};
    const char* position_end{current};
    char fill_intermediate{0};

    while (current not_eq end) {
      switch (state) {
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
        }
        case State::End: {
          // throw ::std::runtime_error(
          //     "Unexpected additional characters found in format specifier");
        }
      }
      ++current;
    }
    if (position_end) {
      position_ = to_number(fmt.data(), position_end);
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
  ::std::size_t position_{0};
  ::std::size_t specifiers_{0};
  ::std::size_t size_{0};
  bool has_position_{false};
  bool has_size_{false};
  char fill_{' '};
};

class BasicAppendable {
 public:
  constexpr BasicAppendable() = default;
  virtual ~BasicAppendable() = default;
  virtual void append(::std::string& str) const = 0;
};

template <typename Type>
class Appendable;

template <>
class Appendable<const char*> : public BasicAppendable {
 public:
  constexpr explicit Appendable(const char* val) : val_(val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  void append(::std::string& str) const override { str.append(val_); }

 private:
  const char* val_;
};
template <>
class Appendable<::std::string> : public BasicAppendable {
 public:
  constexpr explicit Appendable(const ::std::string& val) : val_(&val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  void append(::std::string& str) const override { str.append(*val_); }

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
  void append(::std::string& str) const override {
    str.append(::std::to_string(val_));
  }

 private:
  T val_;
};

class FormatWeave {
 public:
  constexpr explicit FormatWeave(const ::std::string_view fmt)
      : kind_{Kind::FormatString}, payload_{.format_string_ = fmt} {}

  constexpr explicit FormatWeave(const BasicAppendable* const appendable)
      : kind_{Kind::Appendable}, payload_{.appendable_ = appendable} {}

  constexpr inline auto append(::std::string& str) const -> void {
    switch (kind_) {
      case Kind::FormatString: {
        str.append(payload_.format_string_);
        break;
      }
      case Kind::Appendable: {
        if (payload_.appendable_) {
          payload_.appendable_->append(str);
        }
        break;
      }
    }
  }

  constexpr ~FormatWeave() = default;

 private:
  enum class Kind { FormatString, Appendable };

  constexpr inline auto set_format_string(const ::std::string_view fmt)
      -> void {
    kind_ = Kind::FormatString;
    payload_.format_string_ = fmt;
  }
  constexpr inline auto set_appendable(const BasicAppendable* const appendable)
      -> void {
    kind_ = Kind::Appendable;
    payload_.appendable_ = appendable;
  }

  struct {
    Kind kind_;
    union {
      ::std::string_view format_string_;
      const BasicAppendable* appendable_;
    } payload_;
  };
};

template <typename... ArgsType>
class FormatString {
  static constexpr auto Arity = parameter_pack_arity<ArgsType...>();

 public:
  consteval FormatString(const ::std::string_view fmt) : fmt_{fmt} {  // NOLINT
    verify_arg_count();
  }
  constexpr FormatString() = default;
  constexpr ~FormatString() = default;

  constexpr inline auto get_arg_specifiers() const noexcept
      -> const ::std::array<FormatSpecifier, Arity>& {
    return args_;
  }

 private:
  consteval inline auto count_format_args() -> ::std::size_t {
    constexpr auto npos{::std::string_view::npos};  // NOLINT

    const char* current{fmt_.data()};
    const char* const end{fmt_.data() + fmt_.length()};

    ::std::size_t count{0};

    while (current not_eq end) {
      auto left{::std::string_view{current, end}.find_first_of('{')};
      if (left not_eq npos) {
        auto right{::std::string_view{current, end}.find_first_of('}')};
        if (right == npos) {
          _throw_format_error("Missing closing brace");
        }
        ::std::string_view format_specifier_str{current + left + 1,
                                                right - left - 1};
        FormatSpecifier specifier{format_specifier_str};
        args_.at(count++) = specifier;

        current += right + 1;
      } else {
        current = end;
      }
    }
    return count;
  }

  consteval inline auto verify_arg_count() -> void {
    [[maybe_unused]] auto count{count_format_args()};
  }

  ::std::string_view fmt_;
  ::std::array<FormatSpecifier, Arity> args_;
};

template <typename... ArgsType>
constexpr auto estimate_space(const FormatArgs<ArgsType...> args)
    -> ::std::size_t {
  return args.estimate_size();
}

template <typename Type>
constexpr inline auto make_appendable(const Type& val) -> BasicAppendable* {
  return new Appendable<Type>(val);
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
constexpr inline auto verify_arg_count(
    const ::std::string_view fmt,
    [[maybe_unused]] const FormatArgs<ArgsType...>& args) -> void {
  constexpr ::std::size_t ParamCount{parameter_pack_arity<ArgsType...>()};
  ::std::size_t arg_count{count_placeholders(fmt)};

  // TODO : FIX THIS
  // if (arg_count not_eq ParamCount) {
  //   throw ::std::runtime_error("Incorrect number of arguments");
  // }
}

template <typename... ArgsType>
constexpr auto format(const ::std::string_view fmt,
                      ArgsType... args_pack) -> ::std::string {
  const FormatArgs<ArgsType...> args{::std::forward<ArgsType>(args_pack)...};
  constexpr ::std::size_t ParamCount{parameter_pack_arity<ArgsType...>()};

  verify_arg_count(fmt, args);

  return Formatter{fmt}(args);
}

}  // namespace format

#endif  // FORMAT_FORMAT_HPP_
