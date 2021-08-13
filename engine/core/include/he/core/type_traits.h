// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

#include <type_traits>

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Range Providers

    template <typename T, typename E>
    struct _ProvidesStdContiguousRange
    {
        template <typename U, class D = RemovePointer<decltype(DeclVal<U&>().data())>, class S = decltype(DeclVal<U&>().size())>
        static std::true_type Test(std::enable_if_t<std::is_convertible_v<D(*)[], E(*)[]> && std::is_convertible_v<S, size_t>, U*>);

        template <typename U>
        static std::false_type Test(...);
    };
    template <typename T, typename E> inline constexpr bool ProvidesStdContiguousRange = decltype(_ProvidesStdContiguousRange<T, E>::template Test<T>(nullptr))::value;

    template <typename T, class E>
    struct _ProvidesContiguousRange
    {
        template <typename U, class D = RemovePointer<decltype(DeclVal<U&>().Data())>, class S = decltype(DeclVal<U&>().Size())>
        static std::true_type Test(std::enable_if_t<std::is_convertible_v<D(*)[], E(*)[]> && std::is_convertible_v<S, uint32_t>, U*>);

        template <typename U>
        static std::false_type Test(...);
    };
    template <typename T, typename E> inline constexpr bool ProvidesContiguousRange = decltype(_ProvidesContiguousRange<T, E>::template Test<T>(nullptr))::value;

    // --------------------------------------------------------------------------------------------
    // Is Specialization

    template <typename T, template <typename...> typename Template>
    struct _IsSpecialization : std::false_type {};

    template <template <typename...> typename Template, typename... Types>
    struct _IsSpecialization<Template<Types...>, Template> : std::true_type {};

    template <typename T, template <typename...> typename Template> inline constexpr bool IsSpecialization = _IsSpecialization<T, Template>::value;

    // --------------------------------------------------------------------------------------------
    // Enums

    template <typename T> inline constexpr bool IsEnum = __is_enum(T);
    template <typename T> using EnumType = __underlying_type(T);
}

#define HE_REQUIRED(...) std::enable_if_t<(__VA_ARGS__), decltype(nullptr)>
#define HE_REQUIRES(...) HE_REQUIRED(__VA_ARGS__) = nullptr
