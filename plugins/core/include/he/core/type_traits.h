// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

#include <concepts>
#include <type_traits>

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Aligned Storage

    /// A trivial standard-layout type suitable for use as uninitialized storage for any object
    /// whose size is at most `Len` and whose alignment requirement is a divisor of `Align`.
    ///
    /// \note Behavior is undefined if `Len == 0`
    ///
    /// \tparam Len The number of bytes to provide storage for.
    /// \tparam Align The alignment the bytes must satisfy.
    template <uint32_t Len, uint32_t Align>
    struct AlignedStorage
    {
        static constexpr uint32_t Length = Len;
        static constexpr uint32_t Alignment = Align;
        alignas(Alignment) uint8_t data[Len];
    };

    /// A trivial standard-layout type suitable for use as uninitialized storage for an object
    /// of type `T`.
    ///
    /// \tparam T The type to provide aligned storage for.
    template <typename T>
    using AlignedStorageFor = AlignedStorage<sizeof(T), alignof(T)>;

    // --------------------------------------------------------------------------------------------
    // Is Specialization

    template <typename T, template <typename...> typename Template>
    struct _IsSpecialization : std::false_type {};

    template <template <typename...> typename Template, typename... Types>
    struct _IsSpecialization<Template<Types...>, Template> : std::true_type {};

    template <typename T, template <typename...> typename Template>
    inline constexpr bool IsSpecialization = _IsSpecialization<T, Template>::value;

    // --------------------------------------------------------------------------------------------
    // Array Element

    template <typename T>
    struct _ArrayTraits {};

    template <typename T, size_t N>
    struct _ArrayTraits<T[N]> { using ElementType = T; };

    template <typename T>
    struct _ArrayTraits<T[]> { using ElementType = T; };

    template <typename T>
    using ArrayElementType = typename _ArrayTraits<T>::ElementType;

    // --------------------------------------------------------------------------------------------
    // Remove R-Value Reference

    template <typename T>
    struct _RemoveRValueReference { using Type = T; };

    template <typename T>
    struct _RemoveRValueReference<T&&> { using Type = T; };

    template <typename T>
    using RemoveRValueReference = typename _RemoveRValueReference<T>::Type;

    // --------------------------------------------------------------------------------------------
    // Constness As

    template <typename To, typename From>
    using ConstnessAs = std::conditional_t<std::is_const_v<From>, std::add_const_t<To>, std::remove_const_t<To>>;

    // --------------------------------------------------------------------------------------------
    // Function pointer type deduction

    template <typename R, typename... Args>
    constexpr auto _FunctionPointer(R(*)(Args...)) -> R(*)(Args...);

    template <typename R, typename T, typename... Args, typename Rest>
    constexpr auto _FunctionPointer(R(*)(T, Args...), Rest&&) -> R(*)(Args...);

    template <typename T, typename R, typename... Args, typename... Rest>
    constexpr auto _FunctionPointer(R(T::*)(Args...), Rest&&...) -> R(*)(Args...);

    template <typename T, typename R, typename... Args, typename... Rest>
    constexpr auto _FunctionPointer(R(T::*)(Args...) const, Rest&&...) -> R(*)(Args...);

    template <typename T, typename R, typename... Rest>
    constexpr auto _FunctionPointer(R T::*, Rest&&...) -> R(*)();

    template <typename... T>
    using FunctionPointer = decltype(_FunctionPointer(std::declval<T>()...));

    // --------------------------------------------------------------------------------------------
    // Structure for passing around types

    template <typename... T>
    struct TypeList
    {
        using Type = TypeList;
        static constexpr auto Size = sizeof...(T);
    };

    template <size_t, typename>
    struct _TypeListElement;

    template <size_t Index, typename First, typename... Rest>
    struct _TypeListElement<Index, TypeList<First, Rest...>>
        : _TypeListElement<Index - 1u, TypeList<Rest...>> {};

    template <typename First, typename... Rest>
    struct _TypeListElement<0u, TypeList<First, Rest...>>
    {
        using Type = First;
    };

    template <size_t Index, typename List>
    using TypeListElement = typename _TypeListElement<Index, List>::Type;

    // --------------------------------------------------------------------------------------------
    // Concepts

    template <typename T>
    concept Arithmetic = std::is_arithmetic_v<T>;

    template <typename T>
    concept Enum = std::is_enum_v<T>;

    template <typename T, typename U>
    concept Exactly = std::is_same_v<T, U>;

    template <typename T, typename... U>
    concept AnyOf = (std::same_as<T, U> || ...);

    template <typename T, typename E>
    concept ContiguousRange = requires(T & t)
    {
        { t.Data() } -> std::convertible_to<std::add_pointer_t<E>>;
        { t.Size() } -> std::convertible_to<uint32_t>;
    };

    template <typename T, typename E>
    concept StdContiguousRange = requires(T & t)
    {
        { t.data() } -> std::convertible_to<std::add_pointer_t<E>>;
        { t.size() } -> std::convertible_to<size_t>;
    };

    template <typename T>
    concept ArithmeticRangePtr = std::is_pointer_v<T> && std::is_arithmetic_v<std::remove_pointer_t<T>>;

    template <typename T>
    concept ArithmeticRange = requires(T & t)
    {
        { t.Data() } -> ArithmeticRangePtr;
        { t.Size() } -> std::convertible_to<uint32_t>;
    };

    template <typename T>
    concept StdArithmeticRange = requires(T & t)
    {
        { t.data() } -> ArithmeticRangePtr;
        { t.size() } -> std::convertible_to<size_t>;
    };
}
