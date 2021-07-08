// Copyright Chad Engler

#pragma once

#include "he/core/span.h"

namespace he
{
    class String;

    /// A string view wraps access to a constant contiguous range of characters.
    /// StringView is much like a Span<>, but with string-specific semantics.
    class StringView final
    {
    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty string view.
        constexpr StringView() = default;

        /// Construct a string view from a string object.
        ///
        /// \param str The string to refer to.
        StringView(const String& str);

        /// Construct a string view from a null terminated string.
        ///
        /// \param str The string to refer to.
        StringView(const char* str);

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

        /// Construct a span from another span object.
        ///
        /// \param x The string view to refer to.
        constexpr StringView(const StringView& x)
            : m_span(x.m_span)
        {}

        // ----------------------------------------------------------------------------------------
        // Operators

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
        bool operator==(const String& x);

        /// Checks if this string is not equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are not equal, false otherwise.
        bool operator!=(const String& x);

        /// Checks if this string is less than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than `x`, false otherwise.
        bool operator<(const String& x);

        /// Checks if this string is less than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than or equal to `x`, false otherwise.
        bool operator<=(const String& x);

        /// Checks if this string is greater than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than `x`, false otherwise.
        bool operator>(const String& x);

        /// Checks if this string is greater than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than or equal to `x`, false otherwise.
        bool operator>=(const String& x);

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
        // Comparison

        /// Compares this string view to another and returns the result of the comparison.
        ///
        /// \param x The string view to compare against.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        int32_t CompareTo(const StringView& x);

        /// Compares this string view to another and returns true if they are equal.
        ///
        /// \param x The string to compare against.
        /// \return True if the strings are equal, false otherwise.
        bool EqualTo(const StringView& x);

        /// Compares this string view to another and returns true if this string is less than `x`.
        ///
        /// \param x The string to compare against.
        /// \return True if this string is less than `x`, false otherwise.
        bool LessThan(const StringView& x);

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
        Span<const char> m_span;
    };
}
