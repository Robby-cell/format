#ifndef FORMAT_FORMAT_HPP_
#define FORMAT_FORMAT_HPP_

#include <array>
#include <cstring>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace format {

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

struct ReplacementSpan {
 public:
  constexpr auto begin() -> ::std::size_t { return begin_; }
  constexpr auto end() -> ::std::size_t { return end_; }
  constexpr auto span() -> ::std::size_t { return end_ - begin_; }

  constexpr auto operator=(const ReplacementSpan& rhs) -> ReplacementSpan& =
                                                              default;
  constexpr auto operator=(ReplacementSpan&& rhs) -> ReplacementSpan& = default;

  constexpr ReplacementSpan(::std::size_t begin, ::std::size_t end)
      : begin_(begin), end_(end) {}
  constexpr ReplacementSpan() = default;

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

template <typename... ArgsType>
constexpr auto _substitute(const ::std::string_view fmt,
                           const FormatArgs<ArgsType...>& args)
    -> ::std::string {
  ::std::string out{estimate_space(args) + fmt.length()};

  return out;
}

template <typename... ArgsType>
constexpr inline auto verify_arg_count(
    const ::std::string_view fmt,
    [[maybe_unused]] const FormatArgs<ArgsType...>& args) -> void {
  constexpr ::std::size_t ParamCount{parameter_pack_arity<ArgsType...>()};
  ::std::size_t arg_count{count_placeholders(fmt)};

  if constexpr (arg_count not_eq ParamCount) {
    throw ::std::runtime_error("Incorrect number of arguments");
  }
}

template <typename... ArgsType>
constexpr auto format(const ::std::string_view fmt,
                      ArgsType&&... args_pack) -> ::std::string {
  const FormatArgs<ArgsType...> args{::std::forward<ArgsType>(args_pack)...};
  constexpr ::std::size_t ParamCount{parameter_pack_arity<ArgsType...>()};

  verify_arg_count(fmt, args);

  return _substitute(fmt, args);
}

}  // namespace format

namespace std {
template <::std::size_t Index, typename... Args>
constexpr auto get(const format::FormatArgs<Args...>& fmt) {
  return ::std::get<Index>(fmt.args_);
}
}  // namespace std

#endif  // FORMAT_FORMAT_HPP_
