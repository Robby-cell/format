#ifndef FORMAT_CONCEPT_HPP_
#define FORMAT_CONCEPT_HPP_

#include <cstddef>
#include <cstdint>

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

// TODO : APPENDABLE

template <typename... Types>
struct TypeList;

using UnsignedIntType = TypeList<unsigned, u8, u16, u32, u64, usize>;
using UnsignedIntNoCharType = TypeList<unsigned, u16, u32, u64, usize>;
using SignedIntType = TypeList<int, i8, i16, i32, i64, isize>;
using SignedIntNoCharType = TypeList<int, i16, i32, i64, isize>;
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

}  // namespace fmt

#endif  // FORMAT_CONCEPT_HPP_
