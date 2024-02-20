// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/concepts.h"
#include "he/core/compiler.h"
#include "he/core/hash.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"

HE_BEGIN_NAMESPACE_STD
struct bidirectional_iterator_tag;
HE_END_NAMESPACE_STD

namespace he
{
    // --------------------------------------------------------------------------------------------
    template <typename T>
    class BitReference
    {
    public:
        constexpr BitReference& operator=(bool value) noexcept { m_container->Set(m_index, value); return *this; }
        [[nodiscard]] constexpr bool operator~() const noexcept { return !m_container->IsSet(m_index); }
        [[nodiscard]] constexpr operator bool() const noexcept { return m_container->IsSet(m_index); }

        constexpr BitReference& Flip() noexcept { m_container->Flip(m_index); return *this; }

    private:
        friend T;

        constexpr BitReference() = default;
        constexpr BitReference(T& container, uint32_t index) : m_container(&container), m_index(index) {}

        T* m_container{ nullptr };
        uint32_t m_index{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class BitIterator
    {
    public:
        using ElementType = Conditional<IsConst<T>, bool, BitReference<T>>;

        using difference_type = uint32_t;
        using value_type = ElementType;
        using container_type = T;
        using iterator_category = std::bidirectional_iterator_tag;
        using _Unchecked_type = BitIterator; // Mark iterator as checked.

    public:
        constexpr BitIterator() = default;
        constexpr BitIterator(T& container, uint32_t index) noexcept : m_container(&container), m_index(index) {}

        constexpr ElementType& operator*() const { return (*m_container)[m_index]; }
        constexpr ElementType operator->() const { return (*m_container)[m_index]; }

        constexpr BitIterator& operator++() { ++m_index; return *this; }
        constexpr BitIterator operator++(int) { BitIterator x(*this); ++m_index; return x; }

        constexpr BitIterator& operator--() { --m_index; return *this; }
        constexpr BitIterator operator--(int) { BitIterator x(*this); --m_index; return x; }

        [[nodiscard]] constexpr BitIterator operator+(uint32_t n) const { BitIterator x(*this); x.m_index += n; return x; }
        [[nodiscard]] constexpr BitIterator operator-(uint32_t n) const { BitIterator x(*this); x.m_index -= n; return x; }

        [[nodiscard]] constexpr bool operator==(const BitIterator& x) const { return m_container == x.m_container && m_index == x.m_index; }
        [[nodiscard]] constexpr bool operator!=(const BitIterator& x) const { return m_container != x.m_container || m_index != x.m_index; }

        [[nodiscard]] constexpr explicit operator bool() const { return m_container != nullptr; }

    private:
        T* m_container{ nullptr };
        uint32_t m_index{ 0 };
    };

    // --------------------------------------------------------------------------------------------

    /// Fixed-size array of bits. This container is non-allocating, the storage for the bits are
    /// in a fixed-size array in the object.
    ///
    /// \tparam N The number of bits in the set.
    template <uint32_t N>
    class BitSet final
    {
    public:
        using ElementType = uint32_t;

        static constexpr uint32_t BitsPerElement = sizeof(ElementType) * 8;
        static constexpr uint32_t BitCount = N;

        static_assert(BitCount > 0, "Number of bits in a bitset must be greater than zero.");

    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Default construct an empty bitset.
        constexpr BitSet() = default;

        /// Construct a bitset by copying another.
        ///
        /// \param[in] x The bitset to copy.
        template <uint32_t X>
        constexpr BitSet(const BitSet<X>& x);

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the bits from another bitset to this one.
        ///
        /// \param[in] x The bitset to copy.
        template <uint32_t X>
        constexpr BitSet<N>& operator=(const BitSet<X>& x);

        /// Gets a bool representing if the bit at `index` is set. Asserts if `index` is not less
        /// than \ref Size().
        ///
        /// \param[in] index The index of the bit to check.
        /// \return True if the bit is set, false otherwise.
        [[nodiscard]] constexpr bool operator[](uint32_t index) const { return IsSet(index); }

        /// Gets a reference object referring to the bit at `index`. Asserts if `index` is not less
        /// than \ref Size().
        ///
        /// \param[in] index The index of the bit to get a reference to.
        /// \return A \ref BitReference object referring to the bit at `index`.
        [[nodiscard]] constexpr BitReference<BitSet> operator[](uint32_t index);

        /// Checks if this bitset is equal to `x`.
        ///
        /// \param[in] x The bitset to check against.
        /// \return True if the bitsets are equal, false otherwise.
        [[nodiscard]] constexpr bool operator==(const BitSet& x) const;

        /// Checks if this bitset is equal to `x`.
        ///
        /// \param[in] x The bitset to check against.
        /// \return True if the bitsets are equal, false otherwise.
        [[nodiscard]] constexpr bool operator!=(const BitSet& x) const;

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Gets the number of bits in the bitset.
        ///
        /// \return Number of bits in the bitset.
        [[nodiscard]] constexpr uint32_t Size() const noexcept { return BitCount; }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Checks if the bit at the specified index is set.
        ///
        /// \param[in] index The bit index to check.
        /// \return Returns true if the bit is set, false otherwise.
        [[nodiscard]] constexpr bool IsSet(uint32_t index) const;

        /// Returns a 64-bit hash of the bits in the bitset that is suitable for use in hash tables
        /// as a key.
        ///
        /// \return A 64-bit hash of the bits in the bitset.
        [[nodiscard]] uint64_t HashCode() const noexcept { return WyHash::Mem(m_data, DataCount * sizeof(ElementType)); }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets an iterator that refers to the first bit in the bitset.
        ///
        /// \return An iterator referring to the first bit in the bitset.
        constexpr BitIterator<BitSet> Begin() { return BitIterator<BitSet>(*this, 0); }

        /// Gets an iterator that refers to the first bit in the bitset.
        ///
        /// \return An iterator referring to the first bit in the bitset.
        constexpr BitIterator<const BitSet> Begin() const { return BitIterator<const BitSet>(*this, 0); }

        /// Gets an iterator that refers to one-past the last bit in the bitset.
        ///
        /// \return An iterator referring to one-past the last bit in the bitset.
        constexpr BitIterator<BitSet> End() { return BitIterator<BitSet>(*this, BitCount); }

        /// Gets an iterator that refers to one-past the last bit in the bitset.
        ///
        /// \return An iterator referring to one-past the last bit in the bitset.
        constexpr BitIterator<const BitSet> End() const { return BitIterator<const BitSet>(*this, BitCount); }

        /// Gets an iterator that refers to the first bit in the bitset.
        ///
        /// \return An iterator referring to the first bit in the bitset.
        constexpr BitIterator<BitSet> begin() { return Begin(); }

        /// Gets an iterator that refers to the first bit in the bitset.
        ///
        /// \return An iterator referring to the first bit in the bitset.
        constexpr BitIterator<const BitSet> begin() const { return Begin(); }

        /// Gets an iterator that refers to one-past the last bit in the bitset.
        ///
        /// \return An iterator referring to one-past the last bit in the bitset.
        constexpr BitIterator<BitSet> end() { return End(); }

        /// Gets an iterator that refers to one-past the last bit in the bitset.
        ///
        /// \return An iterator referring to one-past the last bit in the bitset.
        constexpr BitIterator<const BitSet> end() const { return End(); }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets, or unsets, the bit at the specified index according to value.
        ///
        /// \param[in] index The bit index to set or unset.
        /// \param[in] value When true will set the bit, when false will unset the bit.
        constexpr void Set(uint32_t index, bool value);

        /// Sets the bit at the specified index.
        ///
        /// \param[in] index The bit index to set.
        constexpr void Set(uint32_t index);

        /// Unsets the bit at the specified index.
        ///
        /// \param[in] index The bit index to unset.
        constexpr void Unset(uint32_t index);

        /// Flips the bit at the specified index.
        ///
        /// \param[in] index The bit index to flip.
        constexpr void Flip(uint32_t index);

        /// Clears the bitset, unsetting all bits.
        ///
        /// \param[in] index The bit index to set.
        constexpr void Clear();

    private:
        static constexpr uint32_t DataCount = (BitCount + (BitsPerElement - 1)) / BitsPerElement;

        ElementType m_data[DataCount]{};
    };

    // --------------------------------------------------------------------------------------------

    /// Like a \ref Span but for individual bits. This container is non-allocating and non-owning.
    /// It is meant to be used as a view into an array of bits allocated elsewhere.
    /// For a fixed-size array of bits use \ref BitSet.
    ///
    /// This class can be useful for dealing with stack-allocated bit arrays, or for dealing with
    /// bit arrays that are embedded in other structures.
    ///
    /// For example:
    /// ```cpp
    /// // Allocate a bit array on the stack, based on a runtime count of values.
    /// const uint32_t elementCount = BitSpan::RequiredElements(count);
    /// BitSpan::ElementType* elements = HE_ALLOCA(BitSpan::ElementType, elementCount);
    /// BitSpan span(elements, elementCount);
    /// ```
    class BitSpan final
    {
    public:
        using ElementType = uint8_t;

        static constexpr uint32_t BitsPerElement = sizeof(ElementType) * 8;

        /// Calculates the number of elements required to store the specified number of bits.
        ///
        /// \param[in] bits The number of bits you wish to store.
        /// \return The number of data elements required to store the specified number of bits.
        static constexpr uint32_t RequiredElements(uint32_t bits) noexcept { return (bits + (BitsPerElement - 1)) / BitsPerElement; }

    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Default construct an empty span.
        constexpr BitSpan() = default;

        /// Construct a span from a pointer and count.
        ///
        /// \param[in] data The pointer to the start of the element range.
        /// \param[in] count The length of the range. This is number of elements, not number of bits.
        constexpr BitSpan(ElementType* data, uint32_t count) noexcept
            : m_data(data)
            , m_size(count)
        {}

        /// Construct a span from a fixed-size array of elements.
        ///
        /// \param[in] arr The array to have the span point to.
        template <uint32_t N>
        constexpr BitSpan(ElementType (&arr)[N]) noexcept
            : m_data(arr)
            , m_size(N)
        {}

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Gets a bool representing if the bit at `index` is set. Asserts if `index` is not less
        /// than \ref Size().
        ///
        /// \param[in] index The index of the bit to check.
        /// \return True if the bit is set, false otherwise.
        [[nodiscard]] constexpr bool operator[](uint32_t index) const { return IsSet(index); }

        /// Gets a reference object referring to the bit at `index`. Asserts if `index` is not less
        /// than \ref Size().
        ///
        /// \param[in] index The index of the bit to get a reference to.
        /// \return A \ref BitReference object referring to the bit at `index`.
        [[nodiscard]] constexpr BitReference<BitSpan> operator[](uint32_t index);

        /// Checks if this span is equal to `x`.
        ///
        /// \param[in] x The span to check against.
        /// \return True if the spans are equal, false otherwise.
        [[nodiscard]] constexpr bool operator==(const BitSpan& x) const;

        /// Checks if this span is equal to `x`.
        ///
        /// \param[in] x The span to check against.
        /// \return True if the spans are equal, false otherwise.
        [[nodiscard]] constexpr bool operator!=(const BitSpan& x) const;

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// The number of bits in the span's range.
        ///
        /// \return Number of bits in the span.
        [[nodiscard]] constexpr uint32_t Size() const noexcept { return m_size * BitsPerElement; }

        /// Checks if this span is empty.
        ///
        /// \return Returns true if this is an empty span.
        [[nodiscard]] constexpr bool IsEmpty() const { return m_size == 0; }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Checks if the bit at the specified index is set.
        ///
        /// \param[in] index The bit index to check.
        /// \return Returns true if the bit is set, false otherwise.
        [[nodiscard]] constexpr bool IsSet(uint32_t index) const;

        /// Returns a 64-bit hash of the bits in the span that is suitable for use in hash tables
        /// as a key.
        ///
        /// \return A 64-bit hash of the bits in the span.
        [[nodiscard]] uint64_t HashCode() const noexcept { return WyHash::Mem(m_data, m_size * sizeof(ElementType)); }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets an iterator that refers to the first bit in the span.
        ///
        /// \return An iterator referring to the first bit in the span.
        constexpr BitIterator<BitSpan> Begin() { return BitIterator<BitSpan>(*this, 0); }

        /// Gets an iterator that refers to the first bit in the span.
        ///
        /// \return An iterator referring to the first bit in the span.
        constexpr BitIterator<const BitSpan> Begin() const { return BitIterator<const BitSpan>(*this, 0); }

        /// Gets an iterator that refers to one-past the last bit in the span.
        ///
        /// \return An iterator referring to one-past the last bit in the span.
        constexpr BitIterator<BitSpan> End() { return BitIterator<BitSpan>(*this, Size()); }

        /// Gets an iterator that refers to one-past the last bit in the span.
        ///
        /// \return An iterator referring to one-past the last bit in the span.
        constexpr BitIterator<const BitSpan> End() const { return BitIterator<const BitSpan>(*this, Size()); }

        /// Gets an iterator that refers to the first bit in the span.
        ///
        /// \return An iterator referring to the first bit in the span.
        constexpr BitIterator<BitSpan> begin() { return Begin(); }

        /// Gets an iterator that refers to the first bit in the span.
        ///
        /// \return An iterator referring to the first bit in the span.
        constexpr BitIterator<const BitSpan> begin() const { return Begin(); }

        /// Gets an iterator that refers to one-past the last bit in the span.
        ///
        /// \return An iterator referring to one-past the last bit in the span.
        constexpr BitIterator<BitSpan> end() { return End(); }

        /// Gets an iterator that refers to one-past the last bit in the span.
        ///
        /// \return An iterator referring to one-past the last bit in the span.
        constexpr BitIterator<const BitSpan> end() const { return End(); }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets, or unsets, the bit at the specified index according to value.
        ///
        /// \param[in] index The bit index to set or unset.
        /// \param[in] value When true will set the bit, when false will unset the bit.
        constexpr void Set(uint32_t index, bool value);

        /// Sets the bit at the specified index.
        ///
        /// \param[in] index The bit index to set.
        constexpr void Set(uint32_t index);

        /// Unsets the bit at the specified index.
        ///
        /// \param[in] index The bit index to unset.
        constexpr void Unset(uint32_t index);

        /// Flips the bit at the specified index.
        ///
        /// \param[in] index The bit index to flip.
        constexpr void Flip(uint32_t index);

        /// Clears the bitspan, unsetting all bits.
        ///
        /// \param[in] index The bit index to set.
        constexpr void Clear();

    private:
        ElementType* m_data{ nullptr };
        uint32_t m_size{ 0 };
    };

    // --------------------------------------------------------------------------------------------

    /// A template class that stores a bitset indexed by an enumeration.
    ///
    /// For example:
    /// ```c++
    /// enum class Test { A, B, C, D };
    /// EnumBitSet<Test> bitset;
    /// bitset.Set(Test::A, Test::B);
    /// bitset.IsSet(Test::A); // true
    /// bitset.Unset(Test::A);
    /// bitset.IsSet(Test::A); // false
    /// ```
    ///
    /// \tparam T The enum type to base the API on.
    template <Enum T>
    class EnumBitSet
    {
    public:
        using EnumType = T;
        using IntType = UnderlyingType<T>;

        static constexpr uint32_t BitSize = sizeof(IntType) * 8;

        static_assert(IsUnsigned<IntType>, "The underlying type of an EnumBitSet must be unsigned.");

        static constexpr IntType Flag(T value) { return (IntType(1) << static_cast<IntType>(value)); }

    public:
        EnumBitSet() = default;

        /// Sets the bit corresponding to the enumeration value.
        ///
        /// \param[in] v The value to set.
        void Set(T v) { m_data |= Flag(v); }

        /// Sets each bit corresponding to each enumeration value.
        ///
        /// \param[in] ...v The values to set.
        void Set(SameAs<T> auto... v) { (Set(v), ...); }

        /// Unsets the bit corresponding to the enumeration value.
        ///
        /// \param[in] v The value to clear.
        void Unset(T v) { m_data &= ~Flag(v); }

        /// Unsets each bit corresponding to each enumeration value.
        ///
        /// \param[in] ...v The values to clear.
        void Unset(SameAs<T> auto... v) { (Unset(v), ...); }

        /// Clears all bits in the bitset.
        ///
        /// \param[in] v The value to set.
        void Clear() { m_data = 0; }

        /// Tests if a bit in the bitset is set.
        ///
        /// \param[in] v The value to test.
        /// \return True if the bit is set, false otherwise.
        bool IsSet(T v) const { return (m_data & Flag(v)) != 0; }

        /// Tests if any of the bits corresponding to the enumeration values are set.
        ///
        /// \param[in] v The value to test.
        /// \return True if any of the values are set, false otherwise.
        bool AreAnySet(T v) const { return IsSet(v); }

        /// \copydoc AreAnySet
        ///
        /// \param[in] ...v The values to test.
        /// \return True if any of the values are set, false otherwise.
        bool AreAnySet(SameAs<T> auto... v) const { return (IsSet(v) || ...); }

        /// Tests if all of the bits corresponding to the enumeration values are set.
        ///
        /// \param[in] v The value to test.
        /// \return True if all of the values are set, false otherwise.
        bool AreAllSet(T v) const { return IsSet(v); }

        /// \copydoc AreAllSet
        ///
        /// \param[in] ...v The values to test.
        /// \return True if all of the values are set, false otherwise.
        bool AreAllSet(SameAs<T> auto... v) const { return (IsSet(v) && ...); }

        /// Gets the raw underlying value of the bitset.
        ///
        /// \return Integer representation of the bitset.
        IntType Value() const { return m_data; }

        /// Gets the number of bits that are controlled by the bitset.
        ///
        /// \return Number of bits in the bitset.
        uint32_t Size() const { return BitSize; }

    private:
        IntType m_data{ 0 };
    };
}

#include "he/core/inline/bitset.inl"
