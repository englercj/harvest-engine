// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"

namespace he
{
    /// A string view wraps access to a constant contiguous range of characters.
    /// StringView is much like a Span<>, but with string-specific semantics.
    class StringView final
    {
    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty string view.
        constexpr StringView() = default;

        /// Construct a string view from a null terminated string.
        ///
        /// \param str The string to refer to.
        constexpr StringView(const char* str)
            : m_span(str, String::LengthConst(str))
        {}

        /// Construct a string view from the range `[begin, end)`.
        ///
        /// \param begin The start of the range for the string view to refer to.
        /// \param end The end of the range for the string view to refer to.
        constexpr StringView(const char* begin, const char* end)
            : m_span(begin, end)
        {}

        /// Construct a string view from `len` chracters of a string.
        ///
        /// \param str The string to refer to.
        /// \param len The number of characters to refer to.
        constexpr StringView(const char* str, uint32_t len)
            : m_span(str, len)
        {}

        /// Construct a string view from a string range provider, such as he::String or std::string.
        ///
        /// \param str The string to refer to.
        template <typename T, HE_REQUIRES(!std::is_same_v<std::remove_cv_t<T>, StringView> && (ProvidesStdContiguousRange<T, const char> || ProvidesContiguousRange<T, const char>))>
        constexpr StringView(const T& str)
            : m_span(str)
        {}

        /// Construct a span from another span object.
        ///
        /// \param x The string view to refer to.
        constexpr StringView(const StringView& x)
            : m_span(x.m_span)
        {}

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the pointer and size of string view `x`.
        ///
        /// \param x The string view to copy from.
        constexpr StringView& operator=(const StringView& x) { m_span = x.m_span; return *this; }

        /// Gets a reference to the character at `index`. Asserts if `index` is not less
        /// than \see Size().
        ///
        /// \param index The index of the character to return.
        /// \return A reference to the character at `index`.
        constexpr const char& operator[](uint32_t index) const { return m_span[index]; }

        /// Checks if this string is equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are equal, false otherwise.
        constexpr bool operator==(const StringView& x) const { return Size() == x.Size() && CompareKnownLength(Data(), x.Data(), Size()) == 0; }

        /// Checks if this string is not equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are not equal, false otherwise.
        constexpr bool operator!=(const StringView& x) const { return !this->operator==(x); }

        /// Checks if this string is less than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than `x`, false otherwise.
        constexpr bool operator<(const StringView& x) const { return CompareTo(x) < 0; }

        /// Checks if this string is less than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than or equal to `x`, false otherwise.
        constexpr bool operator<=(const StringView& x) const { return CompareTo(x) <= 0; }

        /// Checks if this string is greater than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than `x`, false otherwise.
        constexpr bool operator>(const StringView& x) const { return CompareTo(x) > 0; }

        /// Checks if this string is greater than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than or equal to `x`, false otherwise.
        constexpr bool operator>=(const StringView& x) const { return CompareTo(x) >= 0; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if the string this view refers to is empty.
        ///
        /// \return Returns true if this refers to an empty string.
        constexpr bool IsEmpty() const { return m_span.IsEmpty(); }

        /// The length of the string that is refered to.
        ///
        /// \return Number of characters in the view.
        constexpr uint32_t Size() const { return m_span.Size(); }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the start of the string view.
        ///
        /// \return A pointer to the string view's first character.
        constexpr const char* Data() const { return m_span.Data(); }

        /// Gets a reference to the first character in the string view.
        ///
        /// \return A reference to the first character in the view's range.
        constexpr const char& Front() const { return m_span.Front(); }

        /// Gets a reference to the last character in the string view.
        ///
        /// \return A reference to the last character in the view's range.
        constexpr const char& Back() const { return m_span.Back(); }

        // ----------------------------------------------------------------------------------------
        // Converters

        /// Parses the string into a integral value.
        /// If successful, an integer value corresponding to the contents of str is returned.
        /// If the converted value falls out of range of corresponding return type, a range error
        /// occurs (setting errno to ERANGE) and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is
        /// returned. If no conversion can be performed, zero is returned.
        ///
        /// \param[in] str The string to parse.
        /// \param[in] end Optional. A pointer to the end of the string to stop parsing. If
        ///     nullptr (default) the string is parsed until a null terminator is reached.
        /// \param[in] base Optional. The numerical base of the value being parsed.
        /// \return The parsed number.
        template <typename T>
        T ToInteger(int32_t base = 10)
        {
            const char* end = m_span.End();
            return String::ToInteger<T>(m_span.Begin(), &end, base);
        }

        /// Parses the string into a floating point value.
        /// If successful, a floating point value corresponding to the contents of str is returned.
        /// If the converted value falls out of range of corresponding return type, a range error
        /// occurs (setting errno to ERANGE) and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is
        /// returned. If no conversion can be performed, zero is returned.
        ///
        /// \param[in] str The string to parse.
        /// \param[in] end Optional. A pointer to the end of the string to stop parsing. If
        ///     nullptr (default) the string is parsed until a null terminator is reached.
        /// \return The parsed number.
        template <typename T>
        T ToFloat()
        {
            const char* end = m_span.End();
            return String::ToFloat<T>(m_span.Begin(), &end);
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
        constexpr int32_t CompareTo(const StringView& x) const
        {
            const uint32_t s0 = Size();
            const uint32_t s1 = x.Size();
            const int32_t result = CompareKnownLength(Data(), x.Data(), Min(s0, s1));

            if (result != 0)
                return result;

            if (s0 < s1)
                return -1;

            if (s0 > s1)
                return 1;

            return 0;
        }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first character in the string view.
        ///
        /// \return A pointer to the first character.
        constexpr const char* Begin() const { return m_span.Begin(); }

        /// Gets a pointer to one past the last character in the string view.
        ///
        /// \return A pointer to one past the last character.
        constexpr const char* End() const { return m_span.End(); }

        /// \copydoc Begin()
        constexpr const char* begin() const { return m_span.begin(); }

        /// \copydoc End()
        constexpr const char* end() const { return m_span.end(); }

    private:
        /// Compares two strings of known length.
        constexpr int32_t CompareKnownLength(const char* a, const char* b, uint32_t len) const
        {
            for (; len > 0; --len, ++a, ++b)
            {
                if (*a != *b)
                    return *a < *b ? -1 : 1;
            }

            return 0;
        }

    private:
        friend class StringViewTestAttorney;

        Span<const char> m_span;
    };
}
