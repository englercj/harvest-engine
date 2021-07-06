// Copyright Chad Engler

#pragma once

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Constants

    template <typename T, T Val>
    struct IntegralConst { using Type = T; static constexpr T Value = Val; };

    template <bool V>
    using BoolConst = IntegralConst<bool, V>;

    using TrueType  = BoolConst<true>;
    using FalseType = BoolConst<false>;

    // --------------------------------------------------------------------------------------------
    // Enable If

    template <bool Test, typename T = void>
    struct EnableIf_ {};

    template <typename T>
    struct EnableIf_<true, T> { using Type = T; };

    template <bool Test, class T = void>
    using EnableIf = typename EnableIf_<Test, T>::Type;

    // --------------------------------------------------------------------------------------------
    // Triviality

    template <typename T, typename... Args> inline constexpr bool IsTriviallyConstructible = __is_trivially_constructible(T, Args...);
    template <typename T> inline constexpr bool IsTriviallyDestructible = __is_trivially_destructible(T);
    template <typename T> inline constexpr bool IsTriviallyCopyable = __is_trivially_copyable(T);

    // --------------------------------------------------------------------------------------------
    // Remove Reference

    template <typename T> struct RemoveReference_ { using Type = T; };
    template <typename T> struct RemoveReference_<T&> { using Type = T; };
    template <typename T> struct RemoveReference_<T&&> { using Type = T; };

    template <typename T> using RemoveReference = typename RemoveReference_<T>::Type;

    // --------------------------------------------------------------------------------------------
    // Enum

    template <typename T> inline constexpr bool IsEnum = __is_enum(T);
    template <typename T> using EnumType = __underlying_type(T);
}

#define HE_REQUIRED(...) he::EnableIf<(__VA_ARGS__), decltype(nullptr)>
#define HE_REQUIRES(...) HE_REQUIRED(__VA_ARGS__) = nullptr
