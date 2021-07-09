// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he
{
    template <class... T> using Void = void;

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
    struct _EnableIf {};

    template <typename T>
    struct _EnableIf<true, T> { using Type = T; };

    template <bool Test, class T = void>
    using EnableIf = typename _EnableIf<Test, T>::Type;

    // --------------------------------------------------------------------------------------------
    // Triviality

    template <typename T, typename... Args> inline constexpr bool IsTriviallyConstructible = __is_trivially_constructible(T, Args...);
    template <typename T> inline constexpr bool IsTriviallyDestructible = __is_trivially_destructible(T);
    template <typename T> inline constexpr bool IsTriviallyCopyable = __is_trivially_copyable(T);

    // --------------------------------------------------------------------------------------------
    // Remove Reference

    template <typename T> struct _RemoveReference { using Type = T; };
    template <typename T> struct _RemoveReference<T&> { using Type = T; };
    template <typename T> struct _RemoveReference<T&&> { using Type = T; };

    template <typename T> using RemoveReference = typename _RemoveReference<T>::Type;

    // --------------------------------------------------------------------------------------------
    // Add Reference

    template <typename T, typename = void> struct _AddReference { using LVType = T; using RVType = T; };
    template <typename T> struct _AddReference<T, Void<T&>> { using LVType = T&; using RVType = T&&; };

    template <typename T> using AddReference = typename _AddReference<T>::LVType;
    template <typename T> using AddRValueReference = typename _AddReference<T>::RVType;

    // --------------------------------------------------------------------------------------------
    // Remove Pointer

    template <typename T> struct _RemovePointer { using Type = T; };
    template <typename T> struct _RemovePointer<T*> { using Type = T; };
    template <typename T> struct _RemovePointer<T* const> { using Type = T; };
    template <typename T> struct _RemovePointer<T* volatile> { using Type = T; };
    template <typename T> struct _RemovePointer<T* const volatile> { using Type = T; };

    template <typename T> using RemovePointer = typename _RemovePointer<T>::Type;

    // --------------------------------------------------------------------------------------------
    // Add Pointer

    template <typename T, typename = void> struct _AddPointer { using Type = T; };
    template <typename T> struct _AddPointer<T, Void<RemoveReference<T>*>> { using Type = RemoveReference<T>*; };

    template <typename T> using AddPointer = typename _AddPointer<T>::Type;

    // --------------------------------------------------------------------------------------------
    // Remove Const/Volatile

    template <typename T> struct _RemoveCV { using Type = T; };
    template <typename T> struct _RemoveCV<const T> { using Type = T; };
    template <typename T> struct _RemoveCV<volatile T> { using Type = T; };
    template <typename T> struct _RemoveCV<const volatile T> { using Type = T; };

    template <typename T> using RemoveCV = typename _RemoveCV<T>::Type;

    // --------------------------------------------------------------------------------------------
    // Is Convertible

    template <typename From, typename To> inline constexpr bool IsConvertible = __is_convertible_to(From, To);

    // --------------------------------------------------------------------------------------------
    // Is Same

    template <typename, typename> inline constexpr bool IsSame = false;
    template <typename T> inline constexpr bool IsSame<T, T> = true;

    // --------------------------------------------------------------------------------------------
    // Range Providers

    template <typename T, typename El>
    struct _ProvidesStdContiguousRange
    {
        template <typename U, class E = RemovePointer<decltype(DeclVal<U&>().data())>, class S = decltype(DeclVal<U&>().size())>
        static TrueType Test(EnableIf<IsConvertible<E(*)[], El(*)[]> && IsConvertible<S, size_t>, U*>);

        template <typename U>
        static FalseType Test(...);
    };
    template <typename T, typename El> inline constexpr bool ProvidesStdContiguousRange = decltype(_ProvidesStdContiguousRange<T, El>::template Test<T>(nullptr))::Value;

    template <typename T, class El>
    struct _ProvidesContiguousRange
    {
        template <typename U, class E = RemovePointer<decltype(DeclVal<U&>().Data())>, class S = decltype(DeclVal<U&>().Size())>
        static TrueType Test(EnableIf<IsConvertible<E(*)[], El(*)[]> && IsConvertible<S, size_t>, U*>);

        template <typename U>
        static FalseType Test(...);
    };
    template <typename T, typename El> inline constexpr bool ProvidesContiguousRange = decltype(_ProvidesContiguousRange<T, El>::template Test<T>(nullptr))::Value;

    // --------------------------------------------------------------------------------------------
    // Is Specialization

    template <typename T, template <typename...> typename Template>
    struct _IsSpecialization : FalseType {};

    template <template <typename...> typename Template, typename... Types>
    struct _IsSpecialization<Template<Types...>, Template> : TrueType {};

    template <typename T, template <typename...> typename Template> inline constexpr bool IsSpecialization = _IsSpecialization<T, Template>::Value;

    // --------------------------------------------------------------------------------------------
    // declval

    template <typename T> AddRValueReference<T> DeclVal() noexcept;

    // --------------------------------------------------------------------------------------------
    // Enums

    template <typename T> inline constexpr bool IsEnum = __is_enum(T);
    template <typename T> using EnumType = __underlying_type(T);
}

#define HE_REQUIRED(...) he::EnableIf<(__VA_ARGS__), decltype(nullptr)>
#define HE_REQUIRES(...) HE_REQUIRED(__VA_ARGS__) = nullptr
