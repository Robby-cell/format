#ifndef FORMAT_FORMAT_HPP_
#define FORMAT_FORMAT_HPP_

#include <array>
#include <functional>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "format/exception.hpp"
#include "format/formatter.hpp"
#include "format/param.hpp"
#include "format/specifier.hpp"

namespace fmt {

template <typename Type, class Char>
constexpr inline auto buf_print(::std::basic_ostream<Char>& os, const Type& val,
                                const FormatSpecifier& specifiers) -> void {
  Formatter<Type>::buf_print(os, val, specifiers);
}

template <typename Type>
concept BufPrint =
    requires(std::string& str, Type val, const FormatSpecifier& specifier) {
      buf_print(str, val, specifier);
    };

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

  constexpr explicit MappedArgs(const ArgsType&... args)
      : args_(std::make_tuple(std::ref(args)...)) {}

  constexpr MappedArgs() = default;

  template <std::size_t I>
  constexpr auto get() const& -> decltype(auto) {
    return std::get<I>(args_);
  }

 private:
  std::tuple<std::reference_wrapper<const ArgsType>...> args_;
};

template <class Out, class... Args, std::size_t... Is>
auto format_to(Out& os, FormatSpecifier specifier,
               const MappedArgs<Args...>& args,
               std::index_sequence<Is...> /**/) {
  using Dummy = int[];
  (void)Dummy{
      ((specifier.position_ == Is
            ? (buf_print(os, args.template get<Is>().get(), specifier), 0)
            : 0))...};
}

template <class Out, class... Args>
auto format_to(Out& os, FormatSpecifier specifier,
               const MappedArgs<Args...>& args) {
  format_to(os, specifier, args, std::make_index_sequence<sizeof...(Args)>());
}

template <class Out, class Char>
auto format_to(Out& os, const std::basic_string_view<Char> str) {
  os.write(str.data(), str.size());
}

template <class Char, typename... ArgsType>
inline auto _format_impl(const FormatString<ArgsType...>& fmt_str,
                         const MappedArgs<ArgsType...>& args,
                         std::basic_ostream<Char>& out,
                         ::size_t index = 0) -> void {
  ::std::string_view fmt{fmt_str};
  while (not fmt.empty()) {
    const auto left{fmt.find_first_of('{')};
    if (left == ::std::string_view::npos) {
      format_to(out, fmt);
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
    format_to(out, fmt.substr(0, left));
    format_to(out, specifier, args);

    // fmt += right + 1;
    fmt = fmt.substr(right + 1);

    ++index;
  }

  // _format_impl(fmt, args, out, index + 1);
}

template <typename... ArgsType>
[[nodiscard]] constexpr auto format(FormatString<ArgsType...> fmt,
                                    const ArgsType&... args_pack)
    -> ::std::string {
  ::std::stringstream out{};
  const MappedArgs<ArgsType...> mapped_args{args_pack...};

  _format_impl<char>(fmt, mapped_args, out);

  return out.str();
}

}  // namespace fmt

#endif  // FORMAT_FORMAT_HPP_
