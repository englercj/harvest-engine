// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"

#include <type_traits>

#define HE_ENUM_FLAGS(T, ...) \
    HE_PUSH_WARNINGS() \
    HE_DISABLE_GCC_CLANG_WARNING("-Wunused-function") \
    __VA_ARGS__ constexpr T operator~(T a) { using U = std::underlying_type_t<T>; return T(~U(a)); } \
    __VA_ARGS__ constexpr T operator|(T a, T b) { using U = std::underlying_type_t<T>; return T(U(a) | U(b)); } \
    __VA_ARGS__ constexpr T operator&(T a, T b) { using U = std::underlying_type_t<T>; return T(U(a) & U(b)); } \
    __VA_ARGS__ constexpr T operator^(T a, T b) { using U = std::underlying_type_t<T>; return T(U(a) ^ U(b)); } \
    __VA_ARGS__ constexpr T& operator|=(T& a, T b) { using U = std::underlying_type_t<T>; return a = T(U(a) | U(b)); } \
    __VA_ARGS__ constexpr T& operator&=(T& a, T b) { using U = std::underlying_type_t<T>; return a = T(U(a) & U(b)); } \
    __VA_ARGS__ constexpr T& operator^=(T& a, T b) { using U = std::underlying_type_t<T>; return a = T(U(a) ^ U(b)); } \
    HE_POP_WARNINGS()

namespace he
{
    // TODO: Move flag checks into utils.h

    /// Checks if the `value` contains the flag `search`, and only the flag `search`.
    ///
    /// \param value The value to check against.
    /// \param search The flag to search for.
    /// \return True if `value` has the flag `search`.
    template <typename T, typename U = T> requires(std::is_convertible_v<U, T>)
    constexpr bool HasFlag(T value, U search) { return (value & static_cast<T>(search)) == static_cast<T>(search); }

    /// Checks if the `value` contains the flags `search`, and only the flags `search`.
    ///
    /// \param value The value to check against.
    /// \param search The flags to search for.
    /// \return True if `value` has the flags `search`.
    template <typename T, typename U = T> requires(std::is_convertible_v<U, T>)
    constexpr bool HasFlags(T value, U search) { return (value & static_cast<T>(search)) == static_cast<T>(search); }

    /// Checks if the `value` contains the flags `search`.
    ///
    /// \param value The value to check against.
    /// \param search The flags to search for.
    /// \return True if `value` has the flags `search`.
    template <typename T, typename U = T> requires(std::is_convertible_v<U, T>)
    constexpr bool HasAnyFlags(T value, U search) { return static_cast<std::underlying_type_t<T>>(value & static_cast<T>(search)) != 0; }

    /// Returns the enum as a string.
    ///
    /// \param[in] x The value to get the string representation of.
    /// \return The string representation of the enum value.
    template <he::Enum T>
    const char* AsString(T x);
}
