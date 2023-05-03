// Copyright Chad Engler

#pragma once

#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Simple concept versions of type traits

    template <typename T, typename... U>
    concept AllSame = IsAllSame<T, U...>;

    template <typename T, typename... U>
    concept AnyOf =  IsAnyOf<T, U...>;

    template <typename T>
    concept Arithmetic = IsArithmetic<T>;

    template <typename T, typename U>
    concept ConvertibleTo = IsConvertible<T, U>;

    template <typename T, typename U>
    concept AssignableTo = IsAssignable<U, T>;

    template <typename T, typename U>
    concept CopyAssignableTo = IsCopyAssignable<U, T>;

    template <typename T, typename U>
    concept MoveAssignableTo = IsMoveAssignable<U, T>;

    template <typename Derived, typename Base>
    concept DerivedFrom = IsBaseOf<Base, Derived> && IsConvertible<const volatile Derived*, const volatile Base*>;

    template <typename T>
    concept Enum = IsEnum<T>;

    template <typename T>
    concept FloatingPoint = IsFloatingPoint<T>;

    template <typename T>
    concept Integral = IsIntegral<T>;

    template <typename T>
    concept Pointer = IsPointer<T>;

    template <typename T, typename U>
    concept SameAs = IsSame<T, U>;

    template <typename T>
    concept SignedIntegral = IsIntegral<T> && IsSigned<T>;

    template <typename T, template <typename...> typename Template>
    concept SpecializationOf = IsSpecialization<T, Template>;

    template <typename T>
    concept UnsignedIntegral = IsIntegral<T> && IsUnsigned<T>;

    // --------------------------------------------------------------------------------------------
    // Concepts built using other concepts

    template <typename T, typename U>
    concept EqualityComparableWith = requires(const RemoveReference<T>& t, const RemoveReference<U>& u)
    {
        { t == u } -> SameAs<bool>;
        { t != u } -> SameAs<bool>;
        { u == t } -> SameAs<bool>;
        { u != t } -> SameAs<bool>;
    };

    template <typename T>
    concept EqualityComparable = requires(const RemoveReference<T>& t)
    {
        { t == t } -> SameAs<bool>;
        { t != t } -> SameAs<bool>;
    };

    template <typename T>
    concept ContiguousRange = requires(T& t)
    {
        { t.Data() } -> Pointer;
        { t.Size() } -> ConvertibleTo<uint32_t>;
    };

    template <typename T, typename E>
    concept ContiguousRangeOf = requires(T& t)
    {
        { t.Data() } -> ConvertibleTo<E*>;
        { t.Size() } -> ConvertibleTo<uint32_t>;
    };

    template <typename T>
    concept _ArithmeticRangePtr = IsPointer<T> && IsArithmetic<RemovePointer<T>>;

    template <typename T>
    concept ArithmeticRange = requires(T& t)
    {
        { t.Data() } -> _ArithmeticRangePtr;
        { t.Size() } -> ConvertibleTo<uint32_t>;
    };

    // TODO: Generalize ContiguousRangeOf / ArithmeticRange with a concept like this:
    //
    // template <typename T, template <typename> typename Test>
    // concept Matches = Test<T>;
}
