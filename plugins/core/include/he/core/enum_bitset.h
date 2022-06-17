// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/type_traits.h"

namespace he
{
    /// A template class that stores a bitset indexed by an enumeration.
    ///
    /// For example:
    /// ```c++
    /// enum class Test { A, B, C, D };
    /// EnumBitset<Test> bitset;
    /// bitset.Set(Test::A, Test::B);
    /// bitset.IsSet(Test::A); // true
    /// bitset.Unset(Test::A);
    /// bitset.IsSet(Test::A); // false
    /// ```
    ///
    /// \tparam T The enum type to base the API on.
    template <Enum T>
    class EnumBitset
    {
    public:
        using EnumType = T;
        using IntType = std::underlying_type_t<T>;

        static_assert(std::is_unsigned_v<IntType>, "The underlying type of an EnumBitset must be unsigned.");

    public:
        EnumBitset() = default;

        /// Sets the bit corresponding to the enumeration value.
        ///
        /// \param[in] v The value to set.
        void Set(T v) { m_data |= static_cast<IntType>(v); }

        /// Sets each bit corresponding to each enumeration value.
        ///
        /// \param[in] v The value to set.
        /// \param[in] ... Additional values to set.
        void Set(T v, Exactly<T> auto... v2) { Set(v); Set(v2...); }

        /// Unsets the bit corresponding to the enumeration value.
        ///
        /// \param[in] v The value to clear.
        void Unset(T v) { m_data &= ~static_cast<IntType>(v); }

        /// Unsets each bit corresponding to each enumeration value.
        ///
        /// \param[in] v The value to clear.
        /// \param[in] ... Additional values to clear.
        void Unset(T v, Exactly<T> auto... v2) { Unset(v); Unset(v2...); }

        /// Clears all bits in the bitset.
        ///
        /// \param[in] v The value to set.
        void Clear() { m_data = 0; }

        /// Tests if a bit in the bitset is set.
        ///
        /// \param[in] v The value to test.
        /// \return True if the bit is set, false otherwise.
        bool IsSet(T v) const { return (m_data & static_cast<IntType>(v)) != 0; }

        /// Tests if any of the bits corresponding to the enumeration values are set.
        ///
        /// \param[in] v The value to test.
        /// \param[in] ... Additional values to test.
        /// \return True if any of the values are set, false otherwise.
        bool AreAnySet(T v, Exactly<T> auto... v2) const { return IsSet(v) || AreAnySet(v2...); }

        /// Tests if all of the bits corresponding to the enumeration values are set.
        ///
        /// \param[in] v The value to test.
        /// \param[in] ... Additional values to test.
        /// \return True if all of the values are set, false otherwise.
        bool AreAllSet(T v, Exactly<T> auto... v2) const { return IsSet(v) && AreAllSet(v2...); }

        /// Gets the raw underlying value of the bitset.
        ///
        /// \return Integer representation of the bitset.
        IntType Value() const { return m_data; }

    private:
        // basis case for AreAnySet variadic function
        bool AreAnySet(T v) const { return IsSet(v); }

        // basis case for AreAllSet variadic function
        bool AreAllSet(T v) const { return IsSet(v); }

    private:
        IntType m_data{ 0 };
    };
}
