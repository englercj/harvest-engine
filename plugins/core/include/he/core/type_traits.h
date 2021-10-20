// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

#include <concepts>
#include <type_traits>

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Is Specialization

    template <typename T, template <typename...> typename Template>
    struct _IsSpecialization : std::false_type {};

    template <template <typename...> typename Template, typename... Types>
    struct _IsSpecialization<Template<Types...>, Template> : std::true_type {};

    template <typename T, template <typename...> typename Template> inline constexpr bool IsSpecialization = _IsSpecialization<T, Template>::value;

    // --------------------------------------------------------------------------------------------
    // Concepts

    template <typename T, typename E>
    concept ContiguousRange = requires(T& t) {
        { t.Data() } -> std::convertible_to<std::add_pointer_t<E>>;
        { t.Size() } -> std::convertible_to<uint32_t>;
    };

    template <typename T, typename E>
    concept StdContiguousRange = requires(T& t) {
        { t.data() } -> std::convertible_to<std::add_pointer_t<E>>;
        { t.size() } -> std::convertible_to<size_t>;
    };

    template <typename T>
    concept Arithmetic = std::is_arithmetic_v<T>;

    template <typename T>
    concept Enum = std::is_enum_v<T>;
}
