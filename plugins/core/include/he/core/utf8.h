// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/invoke.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/utils.h"

HE_BEGIN_NAMESPACE_STD
struct forward_iterator_tag;
HE_END_NAMESPACE_STD

namespace he
{
    class String;

    /// An invalid unicode code point value used as a sentinel in various functions.
    inline constexpr uint32_t InvalidCodePoint = static_cast<uint32_t>(-1);

    /// Encodes a unicode code point into a UTF-8 multi-byte representation.
    ///
    /// \param[out] dst The destination string to add the code point to.
    /// \param[in] ucc The unicode code point to append.
    /// \return Returns the number of bytes added to `dst`.
    uint32_t UTF8Encode(String& dst, uint32_t ucc);

    /// Encodes a unicode code point into a UTF-8 multi-byte representation.
    ///
    /// \param[out] dst The destination buffer to write the code point to.
    /// \param[in] ucc The unicode code point to append.
    /// \return Returns the number of bytes written to `dst`.
    uint32_t UTF8Encode(char dst[6], uint32_t ucc);

    /// Converts the a valid UTF-8 bytes sequence into a unicode code point.
    ///
    /// \param[out] dst The destination code point to write to. Set to \ref InvalidCodePoint if
    ///     the sequence is invalid or incomplete.
    /// \param[in] str The string to parse.
    /// \param[in] len The length of the input string in bytes.
    /// \return Returns the number of bytes parsed, if the sequence was valid. Zero (`0`) is
    ///     returned if more bytes are required, but the sequence is otherwise valid.
    ///     \ref InvalidCodePoint is returned if the sequence is invalid.
    uint32_t UTF8Decode(uint32_t& dst, const char* str, uint32_t len);

    /// Validates that the incoming string is a valid UTF-8 sequence.
    ///
    /// \param[in] str The string to verify.
    /// \param[in] len The length of the string in bytes.
    /// \return True if the string is valid, false otherwise.
    bool UTF8Validate(const char* str, uint32_t len);

    /// Returns the number of unicode code points in a UTF-8 encoded string.
    ///
    /// \param[in] str The string to count the code points of.
    /// \param[in] len The length of the string in bytes.
    /// \return The number of code points in the string.
    uint32_t UTF8Length(const char* str, uint32_t len);

    /// Returns true if the given unicode code point is a whitespace character.
    ///
    /// \param[in] ucc The unicode code point to check.
    /// \return True if the code point is a whitespace character, false otherwise.
    bool UTF8IsWhitespace(uint32_t ucc);

    /// An iterator for traversing a UTF-8 encoded string.
    class UTF8Iterator
    {
    public:
        using ElementType = uint32_t;

        using difference_type = uint32_t;
        using value_type = ElementType;
        using iterator_category = std::forward_iterator_tag;
        using _Unchecked_type = UTF8Iterator; // Mark iterator as checked.

    public:
        /// Constructs a UTF-8 iterator from a string.
        ///
        /// \param[in] str The string to iterate over.
        explicit UTF8Iterator(StringView str) noexcept : m_str(str) { GoToNext(); }

        /// Returns the string being iterated over.
        ///
        /// \return The string being iterated over.
        const StringView& Str() const { return m_str; }

        /// Returns the next code point without advancing the iterator.
        ///
        /// \return The next code point.
        uint32_t Peek() const { return PeekNext(); }

        /// Returns the current code point.
        ///
        /// \return The current code point.
        uint32_t operator*() const { return m_ucc; }

        /// Advances the iterator to the next code point.
        ///
        /// \return The iterator after advancing.
        UTF8Iterator& operator++() { GoToNext(); return *this; }

        /// Advances the iterator to the next code point.
        ///
        /// \return The iterator before advancing.
        UTF8Iterator operator++(int) { UTF8Iterator x = *this; GoToNext(); return x; }

        /// Advances the iterator by `num` code points.
        ///
        /// \param[in] num The number of code points to advance.
        /// \return The iterator after advancing.
        UTF8Iterator& operator+=(uint32_t num) { while (num) { GoToNext(); --num; } return *this; }

        /// Advances the iterator by `num` code points.
        ///
        /// \param[in] num The number of code points to advance.
        /// \return The iterator before advancing.
        UTF8Iterator operator+(uint32_t num) const { UTF8Iterator x = *this; return (x += num); }

        /// Compares two iterators for equality.
        ///
        /// \param[in] x The iterator to compare against.
        /// \return True if the iterators are equal, false otherwise.
        bool operator==(const UTF8Iterator& x) const { return m_ucc == x.m_ucc && m_str.Size() == x.m_str.Size() && m_str.Data() == x.m_str.Data(); }

        /// Compares two iterators for inequality.
        ///
        /// \param[in] x The iterator to compare against.
        /// \return True if the iterators are not equal, false otherwise.
        bool operator!=(const UTF8Iterator& x) const { return !operator==(x); }

    private:
        uint32_t PeekNext() const
        {
            uint32_t len = 0;
            return PeekNext(len);
        }

        uint32_t PeekNext(uint32_t& len) const
        {
            uint32_t ucc = InvalidCodePoint;

            if (!m_str.IsEmpty()) [[likely]]
            {
                len = UTF8Decode(ucc, m_str.Begin(), m_str.Size());
            }

            return ucc;
        }

        void GoToNext()
        {
            uint32_t len = 0;
            m_ucc = PeekNext(len);
            if (len == 0 || len > m_str.Size()) [[unlikely]]
            {
                m_ucc = InvalidCodePoint;
            }

            if (m_ucc != InvalidCodePoint) [[likely]]
            {
                m_str = { m_str.Begin() + len, m_str.End() };
            }
        }

    private:
        StringView m_str;
        uint32_t m_ucc{ InvalidCodePoint };
    };

    /// An iterator for traversing a UTF-8 encoded string.
    class UTF8ReverseIterator
    {
    public:
        using ElementType = uint32_t;

        using difference_type = uint32_t;
        using value_type = ElementType;
        using iterator_category = std::forward_iterator_tag;
        using _Unchecked_type = UTF8ReverseIterator; // Mark iterator as checked.

    public:
        /// Constructs a UTF-8 iterator from a string.
        ///
        /// \param[in] str The string to iterate over.
        explicit UTF8ReverseIterator(StringView str) noexcept : m_str(str) { GoToPrevious(); }

        /// Returns the string being iterated over.
        ///
        /// \return The string being iterated over.
        const StringView& Str() const { return m_str; }

        /// Returns the next code point without advancing the iterator.
        ///
        /// \return The next code point.
        uint32_t Peek() const { return PeekPrevious(); }

        /// Returns the current code point.
        ///
        /// \return The current code point.
        uint32_t operator*() const { return m_ucc; }

        /// Advances the iterator to the next code point.
        ///
        /// \return The iterator after advancing.
        UTF8ReverseIterator& operator++() { GoToPrevious(); return *this; }

        /// Advances the iterator to the next code point.
        ///
        /// \return The iterator before advancing.
        UTF8ReverseIterator operator++(int) { UTF8ReverseIterator x = *this; GoToPrevious(); return x; }

        /// Advances the iterator by `num` code points.
        ///
        /// \param[in] num The number of code points to advance.
        /// \return The iterator after advancing.
        UTF8ReverseIterator& operator+=(uint32_t num) { while (num) { GoToPrevious(); --num; } return *this; }

        /// Advances the iterator by `num` code points.
        ///
        /// \param[in] num The number of code points to advance.
        /// \return The iterator before advancing.
        UTF8ReverseIterator operator+(uint32_t num) const { UTF8ReverseIterator x = *this; return (x += num); }

        /// Compares two iterators for equality.
        ///
        /// \param[in] x The iterator to compare against.
        /// \return True if the iterators are equal, false otherwise.
        bool operator==(const UTF8ReverseIterator& x) const { return m_ucc == x.m_ucc && m_str.Size() == x.m_str.Size() && m_str.Data() == x.m_str.Data(); }

        /// Compares two iterators for inequality.
        ///
        /// \param[in] x The iterator to compare against.
        /// \return True if the iterators are not equal, false otherwise.
        bool operator!=(const UTF8ReverseIterator& x) const { return !operator==(x); }

    private:
        uint32_t PeekPrevious() const
        {
            uint32_t len = 0;
            return PeekPrevious(len);
        }

        uint32_t PeekPrevious(uint32_t& len) const
        {
            uint32_t ucc = InvalidCodePoint;

            const StringView seq = FindPreviousSequence();
            if (!seq.IsEmpty()) [[likely]]
            {
                len = UTF8Decode(ucc, seq.Begin(), seq.Size());
            }

            return ucc;
        }

        void GoToPrevious()
        {
            uint32_t len = 0;
            m_ucc = PeekPrevious(len);
            if (len == 0 || len > m_str.Size()) [[unlikely]]
            {
                m_ucc = InvalidCodePoint;
            }

            if (m_ucc != InvalidCodePoint) [[likely]]
            {
                m_str = { m_str.Begin(), m_str.End() - len };
            }
        }

        StringView FindPreviousSequence() const
        {
            const char* begin = m_str.End();
            do
            {
                if (begin <= m_str.Begin())
                    return {};

                --begin;
            } while ((*begin & 0xC0) == 0x80);

            return { begin, static_cast<uint32_t>(m_str.End() - begin) };
        }

    private:
        StringView m_str;
        uint32_t m_ucc{ InvalidCodePoint };
    };

    /// Allows iteration of a string by UTF-8 code points.
    class Utf8Splitter
    {
    public:
        /// Constructs a UTF-8 splitter from a string.
        explicit Utf8Splitter(StringView str) : m_str(str) {}

        /// Returns an iterator to the beginning of the string.
        ///
        /// \return The iterator to the beginning of the string.
        UTF8Iterator begin() const { return UTF8Iterator(m_str); }

        /// Returns an iterator to the end of the string.
        ///
        /// \return The iterator to the end of the string.
        UTF8Iterator end() const { return UTF8Iterator({ m_str.End(), m_str.End() }); }

    private:
        const StringView m_str;
    };

    /// Allows iteration of a string by UTF-8 code points, in reverse.
    class Utf8ReverseSplitter
    {
    public:
        /// Constructs a UTF-8 reverse splitter from a string.
        explicit Utf8ReverseSplitter(StringView str) : m_str(str) {}

        /// Returns a reverse iterator to the end of the string.
        ///
        /// \return The reverse iterator to the end of the string.
        UTF8ReverseIterator begin() const { return UTF8ReverseIterator(m_str); }

        /// Returns a reverse iterator to the beginning of the string.
        ///
        /// \return The reverse iterator to the beginning of the string.
        UTF8ReverseIterator end() const { return UTF8ReverseIterator({ m_str.End(), m_str.End() }); }

    private:
        const StringView m_str;
    };

    /// Trims the whitespace from the start of a UTF-8 encoded string.
    ///
    /// \tparam F The function type to use to determine if a code point is whitespace.
    /// \param[in] str The string to trim.
    /// \param[in] isWhitespace Optional. The function to use to determine if a code point is whitespace.
    /// \return The string with the leading whitespace removed.
    template <auto IsWhitespace = UTF8IsWhitespace> requires(IsInvocableR<bool, decltype(IsWhitespace), uint32_t>)
    inline StringView UTF8TrimStart(StringView str)
    {
        uint32_t whitespaceCount = 0;
        for (const uint32_t ucc : Utf8Splitter(str))
        {
            if (!IsWhitespace(ucc))
            {
                break;
            }
            ++whitespaceCount;
        }

        return { str.Begin() + whitespaceCount, str.Size() - whitespaceCount };
    }

    /// Trims the whitespace from the end of a UTF-8 encoded string.
    ///
    /// \tparam F The function type to use to determine if a code point is whitespace.
    /// \param[in] str The string to trim.
    /// \param[in] isWhitespace Optional. The function to use to determine if a code point is whitespace.
    /// \return The string with the trailing whitespace removed.
    template <auto IsWhitespace = UTF8IsWhitespace> requires(IsInvocableR<bool, decltype(IsWhitespace), uint32_t>)
    inline StringView UTF8TrimEnd(StringView str)
    {
        uint32_t whitespaceCount = 0;
        for (const uint32_t ucc : Utf8ReverseSplitter(str))
        {
            if (!IsWhitespace(ucc))
            {
                break;
            }
            ++whitespaceCount;
        }

        return { str.Begin(), str.Size() - whitespaceCount };
    }

    /// Trims the whitespace from the start and end of a UTF-8 encoded string.
    ///
    /// \tparam F The function type to use to determine if a code point is whitespace.
    /// \param[in] str The string to trim.
    /// \param[in] isWhitespace Optional. The function to use to determine if a code point is whitespace.
    /// \return The string with the trailing whitespace removed.
    template <auto IsWhitespace = UTF8IsWhitespace> requires(IsInvocableR<bool, decltype(IsWhitespace), uint32_t>)
    inline StringView UTF8Trim(StringView str)
    {
        str = UTF8TrimStart<IsWhitespace>(str);
        return UTF8TrimEnd<IsWhitespace>(str);
    }
}
