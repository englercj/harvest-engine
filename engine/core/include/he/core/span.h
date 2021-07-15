// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"

namespace he
{
    /// A span wraps access to a contiguous range of objects.
    template <typename T>
    class Span final
    {
    public:
        /// The type of elements in the span.
        using ElementType = T;

        // ----------------------------------------------------------------------------------------
        // Construction

        /// Default construct an empty span.
        constexpr Span() = default;

        /// Construct a span from a pointer and size.
        ///
        /// \param ptr The pointer to the start of the range.
        /// \param size The length of the range.
        constexpr Span(T* ptr, uint32_t size)
            : m_ptr(ptr)
            , m_size(size)
        {}

        /// Construct a span from begin/end pointers. The end pointer is expected to point
        /// to one past the last element of the range.
        ///
        /// \param begin The pointer to the start of the range.
        /// \param end the pointer to one past the last element of the range.
        template <typename P, HE_REQUIRES(IsConvertible<P, T*>)>
        constexpr Span(T* begin, P end)
            : m_ptr(begin)
            , m_size(static_cast<uint32_t>(static_cast<T*>(end) - begin))
        {}

        /// Construct a span from a fixed-size array.
        ///
        /// \param arr The array to have the span point to.
        template <uint32_t N>
        constexpr Span(T (&arr)[N])
            : m_ptr(arr)
            , m_size(N)
        {}

        /// Construct a span from an object that provides a STL-style contiguous range. That is,
        /// it has `.data()` and `.size()` members.
        ///
        /// \param rangeProvider The object that provides the range.
        template <typename R, HE_REQUIRES(!IsSpecialization<RemoveCV<R>, Span> && ProvidesStdContiguousRange<R, T>)>
        constexpr Span(R& rangeProvider)
            : m_ptr(rangeProvider.data())
            , m_size(static_cast<uint32_t>(rangeProvider.size()))
        {
            HE_ASSERT(rangeProvider.size() <= 0xffffffff);
        }

        /// Construct a span from an object that provides a STL-style contiguous range. That is,
        /// it has `.Data()` and `.Size()` members.
        ///
        /// \param rangeProvider The object that provides the range.
        template <typename R, HE_REQUIRES(!IsSpecialization<RemoveCV<R>, Span> && !ProvidesStdContiguousRange<R, T> && ProvidesContiguousRange<R, T>)>
        constexpr Span(R& rangeProvider)
            : m_ptr(rangeProvider.Data())
            , m_size(rangeProvider.Size())
        {}

        /// Construct a span from another span object.
        ///
        /// \param s The span to construct from.
        template <typename U, HE_REQUIRES(IsConvertible<U(*)[], T(*)[]>)>
        constexpr Span(const Span<U>& s)
            : m_ptr(s.m_ptr)
            , m_size(s.m_size)
        {}

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Gets a reference to the element at `index`. Asserts if `index` is not less
        /// than \see Size().
        ///
        /// \param index The index of the element to return.
        /// \return A reference to the element at `index`.
        constexpr T& operator[](uint32_t index) const { HE_ASSERT(index < m_size); return m_ptr[index]; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this span is empty.
        ///
        /// \return Returns true if this is an empty span.
        constexpr bool IsEmpty() const { return m_size == 0; }

        /// The length of the span's range.
        ///
        /// \return Number of element in the range.
        constexpr uint32_t Size() const { return m_size; }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets the span's pointer to the start of the range.
        ///
        /// \return A pointer to the start of the span's range.
        constexpr T* Data() const { return m_ptr; }

        /// Gets a reference to the first element in the span.
        ///
        /// \return A reference to the first element in the span's range.
        constexpr T& Front() const { HE_ASSERT(m_size > 0); return *m_ptr; }

        /// Gets a reference to the last element in the span.
        ///
        /// \return A reference to the last element in the span's range.
        constexpr T& Back() const { HE_ASSERT(m_size > 0); return m_ptr[m_size - 1]; }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first element in the span.
        ///
        /// \return A pointer to the first element.
        constexpr T* Begin() const { return m_ptr; }

        /// Gets a pointer to one past the last element in the span.
        ///
        /// \return A pointer to one past the last element.
        constexpr T* End() const { return m_ptr + m_size; }

        /// \copydoc Begin()
        constexpr T* begin() const { return Begin(); }

        /// \copydoc End()
        constexpr T* end() const { return End(); }

    private:
        template <typename U>
        friend class Span;

        friend class SpanTestAttorney;

        T* m_ptr = nullptr;
        uint32_t m_size = 0;
    };

    /// Helper to create a span from deduced types based on the parameters.
    ///
    /// \param ptr Pointer to the first element of the span.
    /// \param size Number of elements for the span to refer to.
    template <typename T>
    constexpr Span<T> MakeSpan(T* ptr, uint32_t size)
    {
        return { ptr, size };
    }

    /// Helper to create a span from deduced types based on the parameters.
    ///
    /// \param begin Pointer to the first element of the span.
    /// \param end Pointer to one past the last element of the span.
    template <typename T>
    constexpr Span<T> MakeSpan(T* begin, T* end)
    {
        return { begin, end };
    }

    /// Helper to create a span from deduced types based on the parameters.
    ///
    /// \param arr Array to create a span to refer to.
    template <typename T, uint32_t N>
    constexpr Span<T> MakeSpan(T (&arr)[N])
    {
        return { arr };
    }

    /// Creates a span that refers to the bytes of another span.
    ///
    /// \param span The span to get the bytes of.
    template <typename T>
    Span<const uint8_t> AsBytes(const Span<T>& span)
    {
        return Span<const uint8_t>(reinterpret_cast<const uint8_t*>(span.Data()), span.Size() * sizeof(T));
    }
}
