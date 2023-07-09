// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/range_ops.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    /// A fixed sized array of contiguous elements.
    template <typename T, uint32_t N>
    class Array final
    {
        static_assert(N > 0, "Array size must be greater than zero.");

    public:
        /// The type of elements in the array.
        using ElementType = T;

        static constexpr uint32_t SizeValue = N;

    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Default constructor.
        constexpr Array() = default;

        /// Construct an array from a series of values.
        ///
        /// @tparam ...Args The types of the values.
        /// @param ...args The values to construct the array with.
        template <typename... Args> requires(IsConstructible<T, Args> && ...)
        constexpr Array(Args&&... args) noexcept : m_data{ Forward<Args>(args)... } {}

        /// Construct an array by copying the elements of `x`.
        ///
        /// \param x The array to copy from.
        constexpr Array(const Array& x) noexcept { RangeCopy(m_data, x.m_data, N); }

        /// Construct an array by moving the elements `x`.
        ///
        /// \param x The array to move from.
        constexpr Array(Array&& x) noexcept { RangeMove(m_data, x.m_data, N); }

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Gets a reference to the element at `index`. Asserts if `index` is not less than
        /// \see Size().
        ///
        /// \param index The index of the element to return.
        /// \return A reference to the element at `index`.
        constexpr const T& operator[](uint32_t index) const { HE_ASSERT(index < N); return m_data[index]; }

        /// \copydoc operator[](uint32_t)
        constexpr T& operator[](uint32_t index) { HE_ASSERT(index < N); return m_data[index]; }

        /// Checks if this array is equal to another array.
        ///
        /// \param x The array to check against.
        /// \return True if the arrays are equal, false otherwise.
        template <typename U> requires(IsConvertible<U(*)[], T(*)[]>)
        constexpr bool operator==(const Array<U, N>& x) const { return RangeEqual(m_data, x.Data(), N); }

        /// Checks if this array is not equal to another array.
        ///
        /// \param x The array to check against.
        /// \return True if the arrays are not equal, false otherwise.
        template <typename U> requires(IsConvertible<U(*)[], T(*)[]>)
        constexpr bool operator!=(const Array<U, N>& x) const { return !this->operator==(x); }

        /// Copy the array elements from `x` into this array.
        ///
        /// \param x The array to copy from.
        Array& operator=(const Array& x) noexcept { RangeCopy(m_data, x.m_data, N); return *this; }

        /// Move the array elements from `x` into this array.
        ///
        /// \param x The array to move from.
        Array& operator=(Array&& x) noexcept { RangeMove(m_data, x.m_data, N); return *this; }

        /// Copies the elements of `range` into this array.
        ///
        /// \param str The array source to copy from.
        template <ContiguousRangeOf<T> R> requires(!IsSame<R, Array<T, N>>)
        Array& operator=(const R& range) { RangeCopy(*this, range); return *this; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// The number of the elements in the array.
        ///
        /// \return Number of elements in the view.
        [[nodiscard]] constexpr uint32_t Size() const { return N; }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the start of the array.
        ///
        /// \return A pointer to the array's first element.
        [[nodiscard]] constexpr const T* Data() const { return m_data; }

        /// \copydoc Data()
        [[nodiscard]] constexpr T* Data() { return m_data; }

        /// Gets a reference to the first element in the array.
        ///
        /// \return A reference to the first element in the view's range.
        [[nodiscard]] constexpr const T& Front() const { return *m_data; }

        /// \copydoc Front()
        [[nodiscard]] constexpr T& Front() { return *m_data; }

        /// Gets a reference to the last element in the array.
        ///
        /// \return A reference to the last element in the view's range.
        [[nodiscard]] constexpr const T& Back() const { return m_data[N - 1]; }

        /// \copydoc Back()
        [[nodiscard]] constexpr T& Back() { return m_data[N - 1]; }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first element in the array.
        ///
        /// \return A pointer to the first element.
        [[nodiscard]] constexpr const T* Begin() const { return m_data; }

        /// \copydoc Begin()
        [[nodiscard]] constexpr T* Begin() { return Begin(); }

        /// Gets a pointer to one past the last element in the array.
        ///
        /// \return A pointer to one past the last element.
        [[nodiscard]] constexpr const T* End() const { return m_data + N; }

        /// \copydoc End()
        [[nodiscard]] constexpr T* End() { return End(); }

        /// \copydoc Begin()
        [[nodiscard]] constexpr const T* begin() const { return Begin(); }

        /// \copydoc Begin()
        [[nodiscard]] constexpr T* begin() { return Begin(); }

        /// \copydoc End()
        [[nodiscard]] constexpr const T* end() const { return End(); }

        /// \copydoc End()
        [[nodiscard]] constexpr T* end() { return End(); }

    private:
        T m_data[N];
    };
}
