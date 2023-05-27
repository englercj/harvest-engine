// Copyright Chad Engler

#pragma once

#include "he/core/concepts.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    /// Returns true if `value` is aligned to `alignment`.
    ///
    /// Assumes that `alignment` is a power of two.
    ///
    /// \param value The value to check for alignment.
    /// \param alignment The alignment to check value for. Must be a power of two.
    /// \return True if the value is aligned, false otherwise.
    template <UnsignedIntegral T>
    [[nodiscard]] constexpr bool IsAligned(T value, size_t alignment) noexcept
    {
        return (value & (alignment - 1)) == 0;
    }

    /// Returns true if `ptr` is aligned to `alignment`.
    ///
    /// Assumes that `alignment` is a power of two.
    ///
    /// \param ptr The pointer to check for alignment.
    /// \param alignment The alignment to check value for. Must be a power of two.
    /// \return True if the pointer is aligned, false otherwise.
    [[nodiscard]] constexpr bool IsAligned(const void* ptr, size_t alignment) noexcept
    {
        return IsAligned(reinterpret_cast<uintptr_t>(ptr), alignment);
    }

    /// Aligns `value` down to the nearest `alignment` increment.
    ///
    /// Assumes that `alignment` is a power of two.
    ///
    /// \param value The value to be aligned down.
    /// \param alignment The alignment to match.
    /// \return The aligned value.
    template <UnsignedIntegral T>
    [[nodiscard]] constexpr T AlignDown(T value, size_t alignment) noexcept
    {
        return static_cast<T>(value & ~(alignment - 1));
    }

    /// Aligns `ptr` down to the nearest `alignment` increment.
    ///
    /// Assumes that `alignment` is a power of two.
    ///
    /// \param ptr The pointer to be aligned down.
    /// \param alignment The alignment to match.
    /// \return The aligned value.
    template <typename T>
    [[nodiscard]] inline T* AlignDown(T* value, size_t alignment) noexcept
    {
        return reinterpret_cast<T*>(AlignDown(reinterpret_cast<uintptr_t>(value), alignment));
    }

    /// Aligns `value` up to the nearest `alignment` increment.
    ///
    /// Assumes that `alignment` is a power of two.
    ///
    /// \param value The value to be aligned up.
    /// \param alignment The alignment to match.
    /// \return The aligned value.
    template <UnsignedIntegral T>
    [[nodiscard]] constexpr inline T AlignUp(T value, size_t alignment) noexcept
    {
        return static_cast<T>((value + (alignment - 1)) & ~(alignment - 1));
    }

    /// Aligns `ptr` up to the nearest `alignment` increment.
    ///
    /// Assumes that `alignment` is a power of two.
    ///
    /// \param ptr The pointer to be aligned up.
    /// \param alignment The alignment to match.
    /// \return The aligned value.
    template <typename T>
    [[nodiscard]] inline T* AlignUp(T* value, size_t alignment) noexcept
    {
        return reinterpret_cast<T*>(AlignUp(reinterpret_cast<uintptr_t>(value), alignment));
    }

    /// Returns true if `value` is a power of two.
    ///
    /// \param value The value to check.
    /// \return True if the value is a power of two, false otherwise.
    template <UnsignedIntegral T>
    [[nodiscard]] constexpr inline bool IsPowerOf2(T value) noexcept
    {
        return value > 0 && (value & (value - 1)) == 0;
    }

    /// Returns the smaller value between `a` and `b`.
    ///
    /// \param a The first value to compare.
    /// \param b The second value to compare.
    /// \return The smaller value.
    template <typename T, typename... Args>
    [[nodiscard]] constexpr T Min(T a, T b, Args... args) noexcept
    {
        if constexpr (sizeof...(args) == 0)
            return a < b ? a : b;
        else
            return Min(Min(a, b), args...);
    }

    /// Returns the larger value between `a` and `b`.
    ///
    /// \param a The first value to compare.
    /// \param b The second value to compare.
    /// \return The larger value.
    template <typename T, typename... Args>
    [[nodiscard]] constexpr T Max(T a, T b, Args... args) noexcept
    {
        if constexpr (sizeof...(args) == 0)
            return a > b ? a : b;
        else
            return Max(Max(a, b), args...);
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

    /// Checks if the `value` contains the flag `search`, and only the flag `search`.
    ///
    /// \param value The value to check against.
    /// \param search The flag to search for.
    /// \return True if `value` has the flag `search`.
    template <typename T, typename U = T> requires(IsConvertible<U, T>)
    [[nodiscard]] constexpr bool HasFlag(T value, U search)
    {
        return (value & static_cast<T>(search)) == static_cast<T>(search);
    }

    /// Checks if the `value` contains the flags `search`, and only the flags `search`.
    ///
    /// \param value The value to check against.
    /// \param search The flags to search for.
    /// \return True if `value` has the flags `search`.
    template <typename T, typename U = T> requires(IsConvertible<U, T>)
    [[nodiscard]] constexpr bool HasFlags(T value, U search)
    {
        return (value & static_cast<T>(search)) == static_cast<T>(search);
    }

    /// Checks if the `value` contains the flags `search`.
    ///
    /// \param value The value to check against.
    /// \param search The flags to search for.
    /// \return True if `value` has the flags `search`.
    template <typename T, typename U = T> requires(IsConvertible<U, T>)
    [[nodiscard]] constexpr bool HasAnyFlags(T value, U search)
    {
        return (value & static_cast<T>(search)) != static_cast<T>(0);
    }

    /// Returns the object's bit pattern as a different type. The two types must both be the
    /// same size, and be trivially copyable.
    ///
    /// \tparam T The type to cast to.
    /// \tparam U The type to cast from, usually this is just deduced from the parameter.
    /// \param src The value to cast to another type.
    /// \return The same bits as a different type.
    template <typename T, class U> requires(sizeof(T) == sizeof(U) && IsTriviallyCopyable<T> && IsTriviallyCopyable<U>)
    [[nodiscard]] constexpr T BitCast(const U& src) noexcept
    {
        return __builtin_bit_cast(T, src);
    }

    /// Forwards lvalues as either lvalues or as rvalues, depending on `T`.
    ///
    /// \param x The object to be forwarded.
    /// \return Cast of the object to an lvalue or rvalue reference.
    template <typename T>
    [[nodiscard]] constexpr T&& Forward(RemoveReference<T>& x) noexcept
    {
        return static_cast<T&&>(x);
    }

    /// Forwards rvalues as rvalues.
    ///
    /// \param x The object to be forwarded.
    /// \return Cast of the object to an lvalue or rvalue reference.
    template <typename T>
    [[nodiscard]] constexpr T&& Forward(RemoveReference<T>&& x) noexcept
    {
        static_assert(IsLValueReference<T>, "Bad forward call.");
        return static_cast<T&&>(x);
    }

    /// Indicate that an object may be moved from.
    ///
    /// \param x The object to be moved.
    /// \return Cast of the object to an rvalue reference.
    template <typename T>
    [[nodiscard]] constexpr RemoveReference<T>&& Move(T&& x) noexcept
    {
        return static_cast<RemoveReference<T>&&>(x);
    }

    /// Exchange the value of `obj` with `newVal` and returns the original value of `obj`.
    ///
    /// \param obj The object to assign the new value to.
    /// \param newVal The new value to assign to `a`.
    /// \return The value previously held in `a`.
    template <typename T, typename U = T>
    constexpr T Exchange(T& obj, U&& newVal) noexcept
    {
        T old = static_cast<T&&>(obj);
        obj = static_cast<U&&>(newVal);
        return old;
    }

    /// Functor template for performing equality comparisons. The base template simply invokes
    /// `operator==` on type `T`.
    template <typename T>
    struct EqualTo
    {
        [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const { return a == b; }
    };

    /// Functor template for performing equality comparisons. The base template simply invokes
    /// `operator!=` on type `T`.
    template <typename T>
    struct NotEqualTo
    {
        [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const { return a != b; }
    };

    /// Functor template for performing less-than comparisons. The base template simply invokes
    /// `operator<` on type `T`.
    template <typename T>
    struct LessThan
    {
        [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const { return a < b; }
    };

    /// Functor template for performing less-than-or-equal comparisons. The base template simply
    /// invokes `operator<=` on type `T`.
    template <typename T>
    struct LessThanOrEqual
    {
        [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const { return a <= b; }
    };

    /// Functor template for performing greater-than comparisons. The base template simply invokes
    /// `operator>` on type `T`.
    template <typename T>
    struct GreaterThan
    {
        [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const { return a > b; }
    };

    /// Functor template for performing greater-than-or-equal comparisons. The base template simply
    /// invokes `operator>=` on type `T`.
    template <typename T>
    struct GreaterThanOrEqual
    {
        [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const { return a >= b; }
    };
}
