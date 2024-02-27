// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

#define HE_ENUM_FLAGS(T) consteval void _heEnableBitmaskEnum(T);

namespace he
{
    template <typename T>
    concept EnumBitmask = IsEnum<T> && requires(T e) { _heEnableBitmaskEnum(e); };

    template <Enum T>
    struct EnumTraits
    {
        using EnumType = T;
        using ValueType = UnderlyingType<T>;

        /// Whether the enum is a bitmask.
        static constexpr bool IsBitmask = EnumBitmask<T>;

        /// Converts an enum value to its underlying type.
        ///
        /// \param[in] x The value to get the value as the underlying type.
        /// \return The enum value as the underlying type.
        static constexpr ValueType ToValue(T x) noexcept { return static_cast<ValueType>(x); }

        /// Returns the enum as a string.
        ///
        /// \param[in] x The value to get the string representation of.
        /// \return The string representation of the enum value.
        static const char* ToString(T x) noexcept;
    };

    template <Enum T>
    const char* EnumToString(T x) noexcept { return EnumTraits<T>::ToString(x); }

    template <Enum T>
    constexpr UnderlyingType<T> EnumToValue(T x) noexcept { return EnumTraits<T>::ToValue(x); }
}

template <he::EnumBitmask T> [[nodiscard]] constexpr T operator~(T a) noexcept { return static_cast<T>(~he::EnumToValue(a)); }
template <he::EnumBitmask T> [[nodiscard]] constexpr T operator|(T a, T b) noexcept { return static_cast<T>(he::EnumToValue(a) | he::EnumToValue(b)); }
template <he::EnumBitmask T> [[nodiscard]] constexpr T operator&(T a, T b) noexcept { return static_cast<T>(he::EnumToValue(a) & he::EnumToValue(b)); }
template <he::EnumBitmask T> [[nodiscard]] constexpr T operator^(T a, T b) noexcept { return static_cast<T>(he::EnumToValue(a) ^ he::EnumToValue(b)); }
template <he::EnumBitmask T> constexpr T& operator|=(T& a, T b) noexcept { return a = static_cast<T>(he::EnumToValue(a) | he::EnumToValue(b)); }
template <he::EnumBitmask T> constexpr T& operator&=(T& a, T b) noexcept { return a = static_cast<T>(he::EnumToValue(a) & he::EnumToValue(b)); }
template <he::EnumBitmask T> constexpr T& operator^=(T& a, T b) noexcept { return a = static_cast<T>(he::EnumToValue(a) ^ he::EnumToValue(b)); }
