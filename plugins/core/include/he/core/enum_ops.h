// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/type_traits.h"

#define HE_ENUM_FLAGS(T) \
    HE_PUSH_WARNINGS() \
    HE_DISABLE_GCC_CLANG_WARNING("-Wunused-function") \
    inline constexpr T operator~(T a) { using U = std::underlying_type_t<T>; return T(~U(a)); } \
    inline constexpr T operator|(T a, T b) { using U = std::underlying_type_t<T>; return T(U(a) | U(b)); } \
    inline constexpr T operator&(T a, T b) { using U = std::underlying_type_t<T>; return T(U(a) & U(b)); } \
    inline constexpr T operator^(T a, T b) { using U = std::underlying_type_t<T>; return T(U(a) ^ U(b)); } \
    inline constexpr T& operator|=(T& a, T b) { using U = std::underlying_type_t<T>; return a = T(U(a) | U(b)); } \
    inline constexpr T& operator&=(T& a, T b) { using U = std::underlying_type_t<T>; return a = T(U(a) & U(b)); } \
    inline constexpr T& operator^=(T& a, T b) { using U = std::underlying_type_t<T>; return a = T(U(a) ^ U(b)); } \
    HE_POP_WARNINGS()

namespace he
{
    /// Returns the enum as a string.
    ///
    /// \param[in] x The value to get the string representation of.
    /// \return The string representation of the enum value.
    template <he::Enum T>
    const char* AsString(T x);
}
