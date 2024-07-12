#ifndef FORMAT_FORMAT_HPP_
#define FORMAT_FORMAT_HPP_

#include <array>
#include <cstring>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace format {

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
  static constexpr size_t Hex{1}, Octal{2}, Binary{3}, Float{4}, Char{5},
      Pointer{6};

  constexpr explicit FormatSpecifier(size_t position, size_t specifiers = 0ULL)
      : position_{position}, specifiers_{specifiers} {}

  /// @brief Default constructor so an array can be created without needing to
  /// initialize all the specifiers
  constexpr FormatSpecifier() = default;
  constexpr ~FormatSpecifier() = default;

 private:
  size_t position_ = 0;
  size_t specifiers_ = 0;
};

template <typename... ArgsType>
class FormatString {
 public:
  consteval FormatString(const ::std::string_view fmt) : fmt_{fmt} {  // NOLINT
    verify_arg_count();
  }
  constexpr FormatString() = default;
  constexpr ~FormatString() = default;

 private:
  consteval inline auto count_format_args() -> ::std::size_t {
    enum class State {
      Base,
      Left,
      Position,
      Specifiers,
      Right,
    } state{State::Base};

    const char* current{fmt_.data()};
    const char* const end{fmt_.data() + fmt_.length()};

    ::std::size_t count{0};

    while (current not_eq end) {
      switch (state) {
        case State::Base: {
          switch (*current) {
            default: {
              break;
            }
            case '{': {
              state = State::Left;
              break;
            }
            case '}': {
              state = State::Right;
              break;
            }
          }
          break;
        }
        case State::Left: {
          switch (*current) {
            default: {
              state = State::Position;
              break;
            }
            case '}': {
              state = State::Base;
              break;
            }
          }
          break;
        }
        case State::Position: {
        }
        case State::Specifiers: {
        }
        case State::Right: {
          if (*current not_eq '}') {
            _throw_format_error(
                "Found a '}' without a matching '{', to have '}' in a format "
                "string, use '}}'");
          }
        }
      }
      ++current;
    }
    return count;
  }
  consteval inline auto verify_arg_count() -> void {
    [[maybe_unused]] auto count{count_format_args()};
  }

  ::std::string_view fmt_;
  ::std::array<FormatSpecifier, parameter_pack_arity<ArgsType...>()> args_;
};

struct ReplacementSpan {
 public:
  constexpr auto begin() const -> ::std::size_t { return begin_; }
  constexpr auto end() const -> ::std::size_t { return end_; }
  constexpr auto span() const -> ::std::size_t { return end_ - begin_; }

  constexpr auto operator=(const ReplacementSpan& rhs) -> ReplacementSpan& =
                                                              default;
  constexpr auto operator=(ReplacementSpan&& rhs) -> ReplacementSpan& = default;

  constexpr ReplacementSpan(::std::size_t begin, ::std::size_t end)
      : begin_(begin), end_(end) {}
  constexpr ReplacementSpan() = default;
  constexpr ReplacementSpan(const ReplacementSpan&) = default;
  constexpr ReplacementSpan(ReplacementSpan&&) = default;

 private:
  ::std::size_t begin_;
  ::std::size_t end_;
};

template <::std::size_t Count>
struct ReplacementList {
 public:
  template <::std::size_t Index>
  constexpr auto get() const noexcept {
    static_assert(Index < Count, "Index must be less than Count");

    return replacements_[Index];
  }

  constexpr ReplacementList() : replacements_{} {}

  constexpr ReplacementList(::std::initializer_list<ReplacementSpan> args)
      : replacements_(
            ::std::forward<::std::initializer_list<ReplacementSpan>>(args)) {}

  template <::std::size_t Index>
  constexpr auto emplace(ReplacementSpan arg) -> void {
    static_assert(Index < Count, "Index must be less than Count");

    replacements_[Index] = arg;
  }

  constexpr auto operator[](::std::size_t index) -> ReplacementSpan& {
    return replacements_[index];
  }

  constexpr auto operator[](::std::size_t index) const
      -> const ReplacementSpan& {
    return replacements_[index];
  }

 private:
  std::array<ReplacementSpan, Count> replacements_;
};

template <typename... ArgsType>
constexpr auto estimate_space(const FormatArgs<ArgsType...> args)
    -> ::std::size_t {
  return args.estimate_size();
}

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
