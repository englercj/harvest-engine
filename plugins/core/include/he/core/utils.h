// Copyright Chad Engler

#pragma once

#include "he/core/type_traits.h"

namespace he
{
    /// Returns true if `value` is aligned to `alignment`.
    ///
    /// Expects that `alignment` is a power of two.
    ///
    /// \param value The value to check for alignment.
    /// \param alignment The alignment to check value for. Must be a power of two.
    /// \return True if the value is aligned, false otherwise.
    template <typename T>
    [[nodiscard]] constexpr bool IsAligned(T value, T alignment) noexcept
    {
        return (value & (alignment - 1)) == 0;
    }

    /// Returns true if `ptr` is aligned to `alignment`.
    ///
    /// Expects that `alignment` is a power of two.
    ///
    /// \param ptr The pointer to check for alignment.
    /// \param alignment The alignment to check value for. Must be a power of two.
    /// \return True if the pointer is aligned, false otherwise.
    [[nodiscard]] inline bool IsAligned(void* ptr, size_t alignment) noexcept
    {
        return IsAligned(reinterpret_cast<uintptr_t>(ptr), alignment);
    }

    /// Returns the aligned value of `value` rounded down based on `alignment`.
    ///
    /// Expects that `alignment` is a power of two.
    ///
    /// \param value The value to be aligned down.
    /// \param alignment The alignment to match.
    /// \return The aligned value.
    template <typename T>
    [[nodiscard]] constexpr T AlignDown(T value, T alignment)
    {
        return value & ~(alignment - 1);
    }

    /// Returns the aligned value of `value` rounded up based on `alignment`.
    ///
    /// Expects that `alignment` is a power of two.
    ///
    /// \param value The value to be aligned up.
    /// \param alignment The alignment to match.
    /// \return The aligned value.
    template <typename T>
    constexpr inline T AlignUp(T value, T alignment)
    {
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    /// Returns the smaller value between `a` and `b`.
    ///
    /// \param a The first value to compare.
    /// \param b The second value to compare.
    /// \return The smaller value.
    template <typename T>
    [[nodiscard]] constexpr T Min(T a, T b) noexcept
    {
        return a < b ? a : b;
    }

    /// Returns the larger value between `a` and `b`.
    ///
    /// \param a The first value to compare.
    /// \param b The second value to compare.
    /// \return The larger value.
    template <typename T>
    [[nodiscard]] constexpr T Max(T a, T b) noexcept
    {
        return a > b ? a : b;
    }

    /// Returns a clamped value between [`min`, `max`].
    ///
    /// \param value The value to clamp.
    /// \param low The lowest of the range `value` can be clamped to.
    /// \param low The highest of the range `value` can be clamped to.
    /// \return The clamped value.
    template <typename T>
    [[nodiscard]] constexpr T Clamp(T value, T low, T high) noexcept
    {
        return value < low ? low : (value > high ? high : value);
    }

    /// Returns the absolute value of `value`.
    ///
    /// \param value The value to get the absolute value of.
    /// \return The absolute value of `value`.
    template <typename T>
    [[nodiscard]] constexpr T Abs(T value) noexcept
    {
        return value < 0 ? -value : value;
    }

    /// Returns the object's bit pattern as a different type. The two types must both be the
    /// same size, and be trivially copyable.
    ///
    /// \tparam T The type to cast to.
    /// \tparam U The type to cast from, usually this is just deduced from the parameter.
    /// \param src The value to cast to another type.
    /// \return The same bits as a different type.
    template <typename T, class U, HE_REQUIRES(sizeof(T) == sizeof(U) && std::is_trivially_copyable_v<T> && std::is_trivially_copyable_v<U>)>
    [[nodiscard]] constexpr T BitCast(const U& src) noexcept
    {
        return __builtin_bit_cast(T, src);
    }

    /// Forwards lvalues as either lvalues or as rvalues, depending on `T`.
    ///
    /// \param x The object to be forwarded.
    /// \return Cast of the object to an lvalue or rvalue reference.
    template <typename T>
    [[nodiscard]] constexpr T&& Forward(std::remove_reference_t<T>& x) noexcept
    {
        return static_cast<T&&>(x);
    }

    /// Indicate that an object may be moved from.
    ///
    /// \param x The object to be moved.
    /// \return Cast of the object to an rvalue reference.
    template <typename T>
    [[nodiscard]] constexpr std::remove_reference_t<T>&& Move(T&& x) noexcept
    {
        return static_cast<std::remove_reference_t<T>&&>(x);
    }

    /// Exchange the value of `obj` with `newVal` and returns the original value of `obj`.
    ///
    /// \param a The object to assign the new value to.
    /// \param b The new value to assign to `a`.
    /// \return The value previously held in `a`.
    template <typename T, class U = T>
    constexpr T Exchange(T& a, U&& b) noexcept
    {
        T old = Move(a);
        a = Forward<U>(b);
        return old;
    }
}
