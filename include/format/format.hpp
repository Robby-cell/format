#ifndef FORMAT_FORMAT_HPP_
#define FORMAT_FORMAT_HPP_

#include <array>
#include <numeric>
#include <type_traits>

#include "format/exception.hpp"
#include "format/formatter.hpp"
#include "format/param.hpp"
#include "format/specifier.hpp"

namespace fmt {

class BasicAppendable {
 public:
  constexpr BasicAppendable() = default;
  virtual ~BasicAppendable() = default;
  virtual constexpr void append(::std::string& str,
                                const FormatSpecifier& specifier) const = 0;
  virtual void stream(std::ostream& os,
                      const FormatSpecifier& specifier) const {
    std::string str;
    append(str, specifier);
    os << str;
  }
};

template <typename Type>
constexpr inline auto buf_print(::std::string& str, const Type& val,
                                const FormatSpecifier& specifiers) -> void {
  Formatter<Type>::buf_print(str, val, specifiers);
}
template <typename Type>
constexpr inline auto buf_print(::std::ostream& os, const Type& val,
                                const FormatSpecifier& specifiers) -> void {
  Formatter<Type>::buf_print(os, val, specifiers);
}

template <typename Type>
concept BufPrint =
    requires(std::string& str, Type val, const FormatSpecifier& specifier) {
      buf_print(str, val, specifier);
    };

template <BufPrint Type>
class Appendable;

template <BufPrint Type>
  requires(::std::is_trivially_copyable_v<Type>)
class Appendable<Type> : public BasicAppendable {
 public:
  constexpr explicit Appendable(const Type val) : val_(val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  constexpr void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    buf_print(str, val_, specifier);
  }

 private:
  const Type val_;
};

template <BufPrint Type>
  requires(not ::std::is_trivially_copyable_v<Type>)
class Appendable<Type> : public BasicAppendable {
 public:
  constexpr explicit Appendable(const Type& val) : val_(&val) {}
  constexpr Appendable() = default;
  constexpr ~Appendable() override = default;
  constexpr void append(
      ::std::string& str,
      [[maybe_unused]] const FormatSpecifier& specifier) const override {
    buf_print(str, *val_, specifier);
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

// template <BufPrint Type>
//   requires(IsIntegerNoChar<Type>)
// class Appendable<Type> : public BasicAppendable {
//  public:
//   constexpr explicit Appendable(Type val) : val_(val) {}
//   constexpr Appendable() = default;
//   constexpr ~Appendable() override = default;
//   constexpr void append(
//       ::std::string& str,
//       [[maybe_unused]] const FormatSpecifier& specifier) const override {
//     if (specifier.is_hex()) {
//       if (specifier.has_size_) {
//         str.append(detail::to_hex(val_, specifier.size_));
//       } else {
//         str.append(detail::to_hex(val_));
//       }
//     } else {
//       str.append(::std::to_string(val_));
//     }
//   }

//  private:
//   Type val_;
// };

// template <BufPrint Type>
//   requires(IsFloat<Type>)
// class Appendable<Type> : public BasicAppendable {
//  public:
//   constexpr explicit Appendable(Type val) : val_(val) {}
//   constexpr Appendable() = default;
//   constexpr ~Appendable() override = default;
//   constexpr void append(
//       ::std::string& str,
//       [[maybe_unused]] const FormatSpecifier& specifier) const override {
//     buf_print(str, val_, specifier);
//   }

//  private:
//   Type val_;
// };

template <typename Type>
  requires(::std::is_trivially_copyable_v<Type>)
constexpr inline auto make_appendable(const Type val) -> BasicAppendable* {
  return new (::std::nothrow) Appendable<Type>(val);
}
template <typename Type>
  requires(not ::std::is_trivially_copyable_v<Type>)
constexpr inline auto make_appendable(const Type& val) -> BasicAppendable* {
  return new (::std::nothrow) Appendable<Type>(val);
}

template <size_t Index = 0, typename... ArgsType>
  requires(Index >= parameter_pack_arity<ArgsType...>())
constexpr inline auto array_fill(
    std::array<BasicAppendable*, parameter_pack_arity<ArgsType...>()>& arr,
    const FormatArgs<const ArgsType*...>& args) noexcept -> void {}

template <size_t Index = 0, typename... ArgsType>
  requires(Index < parameter_pack_arity<ArgsType...>())
constexpr inline auto array_fill(
    std::array<BasicAppendable*, parameter_pack_arity<ArgsType...>()>& arr,
    const FormatArgs<const ArgsType*...>& args) noexcept -> void {
  arr[Index] = make_appendable(*::std::get<Index>(args));
  array_fill<Index + 1, ArgsType...>(arr, args);
}

template <typename... ArgsType>
constexpr inline auto map_args(
    const FormatArgs<const ArgsType*...>& format_args) noexcept
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
  constexpr explicit MappedArgs(const FormatArgs<const ArgsType*...>& args)
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
                                    const ArgsType&... args_pack)
    -> ::std::string {
  const FormatArgs<const ArgsType*...> args{
      ::std::forward<const ArgsType*>(&args_pack)...};

  ::std::string out{};
  out.reserve(args.estimate_size() + fmt.length());

  MappedArgs<ArgsType...> mapped_args{args};

  _format_impl(fmt, mapped_args, out);

  return out;
}

}  // namespace fmt

#endif  // FORMAT_FORMAT_HPP_
