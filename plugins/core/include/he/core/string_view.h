// Copyright Chad Engler

#pragma once

#include "he/core/ascii.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"

namespace he
{
    /// A string view wraps access to a constant contiguous range of characters.
    /// StringView is much like a Span<>, but with string-specific semantics.
    class StringView final
    {
    public:
        /// The type of elements in the string view. Always `char`.
        using ElementType = char;

        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty string view.
        constexpr StringView() = default;

        /// Construct a string view from a null terminated string.
        ///
        /// \param str The string to refer to.
        constexpr StringView(const char* str) noexcept
            : m_data(str)
            , m_size(StrLen(str))
        {}

        /// Construct a string view from the range `[begin, end)`.
        ///
        /// \param begin The start of the range for the string view to refer to.
        /// \param end The end of the range for the string view to refer to.
        constexpr StringView(const char* begin, const char* end) noexcept
            : m_data(begin)
            , m_size(static_cast<uint32_t>(end - begin))
        {}

        /// Construct a string view from `len` chracters of a string.
        ///
        /// \param str The string to refer to.
        /// \param len The number of characters to refer to.
        constexpr StringView(const char* str, uint32_t len) noexcept
            : m_data(str)
            , m_size(len)
        {}

        /// Construct a string view from a string range provider, such as \ref String or
        /// \ref Vector<char>
        ///
        /// \param str The string to refer to.
        template <ContiguousRangeOf<const char> T> requires(!IsSame<RemoveCV<T>, StringView>)
        constexpr StringView(const T& str) noexcept
            : m_data(str.Data())
            , m_size(str.Size())
        {}

        /// Construct a string view from another string view object.
        ///
        /// \param x The string view to refer to.
        constexpr StringView(const StringView& x) noexcept
            : m_data(x.m_data)
            , m_size(x.m_size)
        {}

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the pointer and size of string view `x`.
        ///
        /// \param x The string view to copy from.
        constexpr StringView& operator=(const StringView& x) noexcept
        {
            m_data = x.m_data;
            m_size = x.m_size;
            return *this;
        }

        /// Gets a reference to the character at `index`.
        ///
        /// \param index The index of the character to return.
        /// \return A reference to the character at `index`.
        [[nodiscard]] constexpr char operator[](uint32_t index) const { return m_data[index]; }

        /// Checks if this string is equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are equal, false otherwise.
        [[nodiscard]] bool operator==(const char* x) const { const uint32_t len = StrLen(x); return len == m_size && StrEqualN(m_data, x, m_size); }

        /// Checks if this string is equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are equal, false otherwise.
        [[nodiscard]] constexpr bool operator==(const StringView& x) const { return Size() == x.Size() && CompareInternal(Data(), x.Data(), Size()) == 0; }

        /// Checks if this string is not equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are not equal, false otherwise.
        [[nodiscard]] bool operator!=(const char* x) const { return !this->operator==(x); }

        /// Checks if this string is not equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are not equal, false otherwise.
        [[nodiscard]] constexpr bool operator!=(const StringView& x) const { return !this->operator==(x); }

        /// Checks if this string is less than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than `x`, false otherwise.
        [[nodiscard]] bool operator<(const char* x) const { return StrCompN(m_data, x, m_size) < 0; }

        /// Checks if this string is less than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than `x`, false otherwise.
        [[nodiscard]] constexpr bool operator<(const StringView& x) const { return CompareTo(x) < 0; }

        /// Checks if this string is less than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than or equal to `x`, false otherwise.
        [[nodiscard]] bool operator<=(const char* x) const { return StrCompN(m_data, x, m_size) <= 0; }

        /// Checks if this string is less than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than or equal to `x`, false otherwise.
        [[nodiscard]] constexpr bool operator<=(const StringView& x) const { return CompareTo(x) <= 0; }

        /// Checks if this string is greater than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than `x`, false otherwise.
        [[nodiscard]] bool operator>(const char* x) const { return StrCompN(m_data, x, m_size) > 0; }

        /// Checks if this string is greater than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than `x`, false otherwise.
        [[nodiscard]] constexpr bool operator>(const StringView& x) const { return CompareTo(x) > 0; }

        /// Checks if this string is greater than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than or equal to `x`, false otherwise.
        [[nodiscard]] bool operator>=(const char* x) const { return StrCompN(m_data, x, m_size) >= 0; }

        /// Checks if this string is greater than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than or equal to `x`, false otherwise.
        [[nodiscard]] constexpr bool operator>=(const StringView& x) const { return CompareTo(x) >= 0; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if the string this view refers to is empty.
        ///
        /// \return Returns true if this refers to an empty string.
        [[nodiscard]] constexpr bool IsEmpty() const { return m_size == 0; }

        /// The length of the string that is refered to.
        ///
        /// \return Number of characters in the view.
        [[nodiscard]] constexpr uint32_t Size() const { return m_size; }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the start of the string view.
        ///
        /// \return A pointer to the string view's first character.
        [[nodiscard]] constexpr const char* Data() const { return m_data; }

        /// Gets a reference to the first character in the string view.
        ///
        /// \return A reference to the first character in the view's range.
        [[nodiscard]] constexpr const char& Front() const { return *m_data; }

        /// Gets a reference to the last character in the string view.
        ///
        /// \return A reference to the last character in the view's range.
        [[nodiscard]] constexpr const char& Back() const { return m_data[m_size - 1]; }

        /// Returns a non-cryptographic hash of the string contents.
        ///
        /// \return The hash value.
        [[nodiscard]] uint64_t HashCode() const noexcept;

        /// Searches the string view for a character.
        ///
        /// \param ch The character to search for.
        /// \return A pointer to the found character in the view, or nullptr if not found.
        [[nodiscard]] constexpr const char* Find(char ch) const
        {
            if (IsConstantEvaluated())
            {
                for (const char* p = Begin(); p != End(); ++p)
                {
                    if (*p == ch)
                        return p;
                }

                return nullptr;
            }
            else
            {
                return static_cast<const char*>(MemChr(m_data, ch, m_size));
            }
        }

        // ----------------------------------------------------------------------------------------
        // Converters

        /// Parses the string into a integral value.
        ///
        /// If successful, an integer value corresponding to the contents of `str` is returned.
        /// If the converted value falls out of range of corresponding return type, the value is
        /// clamped to the limits. That is, `Limits<T>::Min` or `Limits<T>::Max` is returned.
        /// If no conversion can be performed, zero is returned.
        ///
        /// \param[in] base Optional. The numerical base of the value being parsed.
        /// \return The parsed number.
        template <typename T>
        [[nodiscard]] T ToInteger(int32_t base = 10) const
        {
            T value = T(0);
            const char* end = End();
            StrToInt(value, Begin(), &end, base);
            return value;
        }

        /// Parses the string into a floating point value.
        ///
        /// If successful, a floating point value corresponding to the contents of str is returned.
        /// If the converted value falls out of range of corresponding return type, the value is
        /// clamped to the limits. That is, `Limits<T>::Min` or `Limits<T>::Max` is returned.
        /// If no conversion can be performed, zero is returned.
        ///
        /// \return The parsed number.
        template <typename T = float>
        [[nodiscard]] T ToFloat() const
        {
            T value = T(0);
            const char* end = End();
            StrToFloat(value, Begin(), &end);
            return value;
        }

        // ----------------------------------------------------------------------------------------
        // Comparison

        /// Compares this string view to another and returns the result of the comparison.
        ///
        /// \param x The string view to compare against.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        [[nodiscard]] constexpr int32_t CompareTo(const StringView& x) const
        {
            const uint32_t s0 = Size();
            const uint32_t s1 = x.Size();
            const int32_t result = CompareInternal(Data(), x.Data(), Min(s0, s1));

            if (result != 0)
                return result;

            if (s0 < s1)
                return -1;

            if (s0 > s1)
                return 1;

            return 0;
        }

        /// Compares this string view to another in a case-insensitive manner and returns the
        /// result of the comparison.
        ///
        /// \param x The string view to compare against.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        [[nodiscard]] constexpr int32_t CompareToI(const StringView& x) const
        {
            const uint32_t s0 = Size();
            const uint32_t s1 = x.Size();
            const uint32_t len = Min(s0, s1);
            const int32_t result = CompareInternalI(Data(), x.Data(), len);

            if (result != 0)
                return result;

            if (s0 < s1)
                return -1;

            if (s0 > s1)
                return 1;

            return 0;
        }

        /// Compares this string view to another and returns true if they are equal.
        ///
        /// \param x The string view to compare against.
        /// \return True if they are equal, false otherwise.
        [[nodiscard]] constexpr bool EqualTo(const StringView& x) const { return CompareTo(x) == 0; }

        /// Compares this string view to another in a case-insensitive manner and
        /// returns true if they are equal.
        ///
        /// \param x The string view to compare against.
        /// \return True if they are equal, false otherwise.
        [[nodiscard]] constexpr bool EqualToI(const StringView& x) const { return CompareToI(x) == 0; }

        /// Checks if this string view starts with the value of `x`.
        ///
        /// @param x The view to check if this starts with.
        /// @return True if this string view starts with the value of `x`, false otherwise.
        [[nodiscard]] constexpr bool StartsWith(const StringView& x) const
        {
            return Size() >= x.Size() ? CompareInternal(Data(), x.Data(), x.Size()) == 0 : false;
        }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first character in the string view.
        ///
        /// \return A pointer to the first character.
        [[nodiscard]] constexpr const char* Begin() const { return m_data; }

        /// Gets a pointer to one past the last character in the string view.
        ///
        /// \return A pointer to one past the last character.
        [[nodiscard]] constexpr const char* End() const { return m_data + m_size; }

        /// \copydoc Begin()
        [[nodiscard]] constexpr const char* begin() const { return Begin(); }

        /// \copydoc End()
        [[nodiscard]] constexpr const char* end() const { return End(); }

    private:
        /// Compares two strings of known length.
        [[nodiscard]] static constexpr int32_t CompareInternal(const char* a, const char* b, uint32_t len)
        {
            if (IsConstantEvaluated())
            {
                for (; len > 0; --len, ++a, ++b)
                {
                    if (*a != *b)
                        return *a < *b ? -1 : 1;
                }

                return 0;
            }
            else
            {
                return MemCmp(a, b, len);
            }
        }

        /// Compares two strings of known length.
        [[nodiscard]] static constexpr int32_t CompareInternalI(const char* a, const char* b, uint32_t len)
        {
            for (; len > 0; --len, ++a, ++b)
            {
                const char al = ToLower(*a);
                const char bl = ToLower(*b);
                if (al != bl)
                    return al < bl ? -1 : 1;
            }

            return 0;
        }

    private:
        friend class StringViewTestAttorney;

        const char* m_data{ nullptr };
        uint32_t m_size{ 0 };
    };

    /// User-defined literal that creates a StringView object from a string literal.
    ///
    /// Example:
    /// ```cpp
    /// constexpr StringView value = "test"_sv;
    /// ```
    ///
    /// \param[in] str A pointer to the string literal.
    /// \param[in] len The length of the string literal, not including the null terminator.
    /// \return A constructed StringView object pointing to the string literal.
    [[nodiscard]] constexpr StringView operator"" _sv(const char* str, size_t len) noexcept
    {
        return StringView(str, static_cast<uint32_t>(len));
    }
}
