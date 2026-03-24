// Copyright Chad Engler

#pragma once

#include "he/core/concepts.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    /// Copies `count` elements from the `src` range to the `dst` range.
    ///
    /// \tparam T The type of the elements in the destination range, usually deduced.
    /// \tparam U The type of the elements in the source range, usually deduced.
    /// \param[out] dst The destination range to copy into.
    /// \param[in] src The source range to copy from.
    /// \param[in] count The number of elements to copy.
    template <typename T, CopyAssignableTo<T> U>
    constexpr void RangeCopy(T* dst, const U* src, uint32_t count);

    /// Copies elements from the `src` range to the `dst` range.
    ///
    /// \tparam R1 The type of the first range, usually deduced.
    /// \tparam R2 The type of the second range, usually deduced.
    /// \param[out] dst The destination range to copy into.
    /// \param[in] src The source range to copy from.
    template <ContiguousRange R1, ContiguousRange R2> requires(IsCopyAssignable<typename R1::ElementType, typename R2::ElementType>)
    constexpr void RangeCopy(R1& dst, const R2& src) { RangeCopy(dst.Data(), src.Data(), Min(dst.Size(), src.Size())); }

    /// Moves `count` elements from the `src` range to the `dst` range.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \param[out] dst The destination range to move into.
    /// \param[in] src The source range to move from.
    /// \param[in] count The number of elements to move.
    template <typename T, MoveAssignableTo<T> U>
    constexpr void RangeMove(T* dst, U* src, uint32_t count);

    /// Moves elements from the `src` range to the `dst` range.
    ///
    /// \tparam R1 The type of the first range, usually deduced.
    /// \tparam R2 The type of the second range, usually deduced.
    /// \param[out] dst The destination range to move into.
    /// \param[in] src The source range to move from.
    template <ContiguousRange R1, ContiguousRange R2> requires(IsMoveAssignable<typename R1::ElementType, typename R2::ElementType>)
    constexpr void RangeMove(R1& dst, R2& src) { RangeMove(dst.Data(), src.Data(), Min(dst.Size(), src.Size())); }

    /// Assigned the `value` to `count` elements in the `dst` range.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \tparam V The type of the value to assign, usually deduced.
    /// \param[in] dst The destination range to assign into.
    /// \param[in] count The number of elements to assign.
    /// \param[in] value The value to assign to the elements.
    template <typename T, ConvertibleTo<T> V>
    constexpr void RangeFill(T* dst, uint32_t count, const V& value);

    /// Assigned the `value` to elements in the `dst` range.
    ///
    /// \tparam R The type of the the range, usually deduced.
    /// \tparam V The type of the value to assign, usually deduced.
    /// \param[in] dst The destination range to assign into.
    /// \param[in] value The value to assign to the elements.
    template <ContiguousRange R, ConvertibleTo<typename R::ElementType> V>
    constexpr void RangeFill(R& dst, const V& value) { RangeFill(dst.Data(), dst.Size(), value); }

    /// Sorts `count` elements in ascending order using `operator<`.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \param[in] begin Pointer to the start of the range to sort.
    /// \param[in] count Number of elements in the range to sort.
    template <typename T> requires(IsConstructible<T, T&&> && IsAssignable<T&, T&&>)
    constexpr void RangeSort(T* begin, uint32_t count);

    /// Sorts `count` elements using the provided comparison predicate.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \tparam F The type of the comparison predicate, usually deduced.
    /// \param[in] begin Pointer to the start of the range to sort.
    /// \param[in] count Number of elements in the range to sort.
    /// \param[in] predicate Comparison predicate matching strict-weak-order semantics.
    template <typename T, typename F> requires(IsConstructible<T, T&&> && IsAssignable<T&, T&&>)
    constexpr void RangeSort(T* begin, uint32_t count, F&& predicate);

    /// Sorts the elements in ascending order using `operator<`.
    ///
    /// \tparam R The type of the range, usually deduced.
    /// \param[in] range The range to sort.
    template <ContiguousRange R>
    constexpr void RangeSort(R& range) { RangeSort(range.Data(), range.Size()); }

    /// Sorts the elements using the provided comparison predicate.
    ///
    /// \tparam R The type of the range, usually deduced.
    /// \tparam F The type of the comparison predicate, usually deduced.
    /// \param[in] range The range to sort.
    /// \param[in] predicate Comparison predicate matching strict-weak-order semantics.
    template <ContiguousRange R, typename F>
    constexpr void RangeSort(R& range, F&& predicate) { RangeSort(range.Data(), range.Size(), Forward<F>(predicate)); }

    /// Stably sorts `count` elements in ascending order using `operator<`.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \param[in] begin Pointer to the start of the range to sort.
    /// \param[in] count Number of elements in the range to sort.
    template <typename T> requires(IsConstructible<T, T&&> && IsAssignable<T&, T&&>)
    constexpr void RangeStableSort(T* begin, uint32_t count);

    /// Stably sorts `count` elements using the provided comparison predicate.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \tparam F The type of the comparison predicate, usually deduced.
    /// \param[in] begin Pointer to the start of the range to sort.
    /// \param[in] count Number of elements in the range to sort.
    /// \param[in] predicate Comparison predicate matching strict-weak-order semantics.
    template <typename T, typename F> requires(IsConstructible<T, T&&> && IsAssignable<T&, T&&>)
    constexpr void RangeStableSort(T* begin, uint32_t count, F&& predicate);

    /// Stably sorts the elements in ascending order using `operator<`.
    ///
    /// \tparam R The type of the range, usually deduced.
    /// \param[in] range The range to sort.
    template <ContiguousRange R>
    constexpr void RangeStableSort(R& range) { RangeStableSort(range.Data(), range.Size()); }

    /// Stably sorts the elements using the provided comparison predicate.
    ///
    /// \tparam R The type of the range, usually deduced.
    /// \tparam F The type of the comparison predicate, usually deduced.
    /// \param[in] range The range to sort.
    /// \param[in] predicate Comparison predicate matching strict-weak-order semantics.
    template <ContiguousRange R, typename F>
    constexpr void RangeStableSort(R& range, F&& predicate) { RangeStableSort(range.Data(), range.Size(), Forward<F>(predicate)); }

    /// Finds a pointer to the first element in the range that is equal to `value`.
    /// Equality is tested using `operator==`.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \tparam V The type of the value to check, usually deduced.
    /// \param[in] begin Pointer to the start of the range to search.
    /// \param[in] count Number of elements in the range to search.
    /// \param[in] value The value to search for.
    /// \return A pointer to the first element that matches `value`, or null if no match is found.
    template <typename T, EqualityComparableWith<T> V>
    constexpr T* RangeFind(T* begin, uint32_t count, const V& value);

    /// Finds a pointer to the first element in the range that is equal to `value`.
    /// Equality is tested using `operator==`.
    ///
    /// \tparam R The type of the the range, usually deduced.
    /// \tparam V The type of the value to check, usually deduced.
    /// \param[in] range The range to search.
    /// \param[in] value The value to search for.
    /// \return A pointer to the first element that matches `value`, or null if no match is found.
    template <ContiguousRange R, EqualityComparableWith<typename R::ElementType> V>
    constexpr typename R::ElementType* RangeFind(R& range, const V& value) { return RangeFind(range.Data(), range.Size(), value); }

    /// Finds a pointer to the first element in the range where `predicate` returns true.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \tparam F The type of the predicate function, usually deduced.
    /// \param[in] begin Pointer to the start of the range to search.
    /// \param[in] count Number of elements in the range to search.
    /// \param[in] predicate The predicate that is used to check if an element matches.
    /// \return A pointer to the first element where `predicate` returns true, or null if no
    /// element matches.
    template <typename T, typename F>
    constexpr T* RangeFindIf(T* begin, uint32_t count, F&& predicate);

    /// Finds a pointer to the first element in the range where `predicate` returns true.
    ///
    /// \tparam R The type of the the range, usually deduced.
    /// \tparam F The type of the predicate function, usually deduced.
    /// \param[in] range The range to search.
    /// \param[in] predicate The predicate that is used to check if an element matches.
    /// \return A pointer to the first element where `predicate` returns true, or null if no
    /// element matches.
    template <ContiguousRange R, typename F>
    constexpr typename R::ElementType* RangeFindIf(const R& range, F&& predicate) { return RangeFindIf(range.Data(), range.Size(), Forward<F>(predicate)); }

    /// Tests if two ranges are equal. Assumes that both ranges are at least `count` elements long.
    /// Equality is tested using `operator==`.
    ///
    /// \tparam T The type of the elements in the range, usually deduced.
    /// \param[in] a Pointer to the start of the first range to check.
    /// \param[in] b Pointer to the start of the second range to check.
    /// \param[in] count Number of elements in the ranges to compare.
    /// \return True if the ranges are equal, false otherwise.
    template <typename T, EqualityComparableWith<T> U>
    constexpr bool RangeEqual(const T* a, const U* b, uint32_t count);

    /// Tests if two ranges are equal, including having the same number of elements.
    /// Equality of elements is tested using `operator==`.
    ///
    /// \tparam R1 The type of the first range, usually deduced.
    /// \tparam R2 The type of the second range, usually deduced.
    /// \param[in] a The first range to check.
    /// \param[in] b The second range to check.
    /// \return True if the ranges are equal, false otherwise.
    template <ContiguousRange R1, ContiguousRange R2> requires(EqualityComparableWith<typename R1::ElementType, typename R2::ElementType>)
    constexpr bool RangeEqual(const R1& a, const R2& b) { return a.Size() == b.Size() && RangeEqual(a.Data(), b.Data(), a.Size()); }

    /// Tests if two ranges are equal. Assumes that both ranges are at least `count` elements long.
    /// Equality is tested using `predicate`.
    ///
    /// \tparam T The type of the elements in the first range, usually deduced.
    /// \tparam U The type of the elements in the second range, usually deduced.
    /// \tparam F The type of the predicate function, usually deduced.
    /// \param[in] a Pointer to the start of the first range to check.
    /// \param[in] b Pointer to the start of the second range to check.
    /// \param[in] count Number of elements in the ranges to compare.
    /// \param[in] predicate The predicate that is used to check if an element matches.
    /// \return True if the ranges are equal, false otherwise.
    template <typename T, typename U, typename F>
    constexpr bool RangeEqual(const T* a, const U* b, uint32_t count, F&& predicate);

    /// Tests if two ranges are equal, including having the same number of elements.
    /// Equality of elements is tested using `predicate`.
    ///
    /// \tparam R1 The type of the first range, usually deduced.
    /// \tparam R2 The type of the second range, usually deduced.
    /// \tparam F The type of the predicate function, usually deduced.
    /// \param[in] a Pointer to the start of the first range to check.
    /// \param[in] b Pointer to the start of the second range to check.
    /// \param[in] count Number of elements in the ranges to compare.
    /// \param[in] predicate The predicate that is used to check if an element matches.
    /// \return True if the ranges are equal, false otherwise.
    template <ContiguousRange R1, ContiguousRange R2, typename F>
    constexpr bool RangeEqual(const R1& a, const R2& b, F&& predicate) { return a.Size() == b.Size() && RangeEqual(a.Data(), b.Data(), a.Size(), Forward<F>(predicate)); }
}

#include "he/core/inline/range_ops.inl"
