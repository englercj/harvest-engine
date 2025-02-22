// Copyright Chad Engler

#include "he/core/kdl_reader.h"

#include "kdl_internal.h"

#include "he/core/allocator.h"
#include "he/core/ascii.h"
#include "he/core/enum_ops.h"
#include "he/core/limits.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/utf8.h"
#include "he/core/vector.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    class KdlParser
    {
    public:
        explicit KdlParser(Allocator& allocator) noexcept
            : m_stringBuffer(allocator)
        {}

        KdlReadResult Parse(StringView src, KdlReader::Handler& handler)
        {
            m_result = {};
            m_handler = &handler;
            m_cursor = src.Begin();
            m_end = src.End();
            m_lineStart = m_cursor;
            m_line = 1;
            m_nodeDepth = 0;
            m_inWhitespaceEscape = false;
            m_stringBuffer.Clear();
            m_typeBuffer.Clear();
            m_slashDashDepthStack.Clear();

            if (!ConsumeBOM())
            {
                return m_result;
            }

            m_handler->StartDocument();

            if (!ConsumeVersion())
            {
                return m_result;
            }

            while (!AtEnd())
            {
                if (!ParseExpression())
                    return m_result;
            }

            // Can't have a dangling slashdash when a document is done
            if (IsAnySlashdashOpen())
            {
                SetError(KdlReadError::InvalidToken);
                return m_result;
            }

            m_handler->EndDocument();

            return m_result;
        }

    private:
        bool SetError(KdlReadError error, char expected = '\0')
        {
            const uint32_t column = UTF8Length(m_lineStart, static_cast<uint32_t>(m_cursor - m_lineStart)) + 1;
            m_result = { error, m_line, column };
            m_result.expected[0] = expected;
            m_result.expected[1] = '\0';
            return false;
        }

        bool SetError(KdlReadError error, uint32_t expected)
        {
            const uint32_t column = UTF8Length(m_lineStart, static_cast<uint32_t>(m_cursor - m_lineStart)) + 1;
            m_result = { error, m_line, column };
            const uint32_t len = UTF8Encode(m_result.expected, expected);
            m_result.expected[len] = '\0';
            return false;
        }

        [[nodiscard]] bool PeekCodePoint(const char* begin, const char* end, uint32_t& ucc, uint32_t& len)
        {
            if (begin >= end || *begin == '\0') [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            len = UTF8Decode(ucc, begin, static_cast<uint32_t>(end - begin));

            // This means that the sequence was valid, but was missing trailing bytes required to
            // properly decode. If we were reading from a stream this would mean we need to read
            // more bytes. Since we're operating on a buffered string, this is an error.
            if (len == 0) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            // This means that the sequence was invalid and could not be decoded.
            if (len == InvalidCodePoint) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidUtf8);
            }

            // Some code points are not allowed in KDL documents.
            if (IsDisallowedKdlCodePoint(ucc)) [[unlikely]]
            {
                return SetError(KdlReadError::DisallowedUtf8);
            }

            return true;
        }

        [[nodiscard]] bool ConsumeCodePoint(const char*& begin, const char* end, uint32_t& ucc)
        {
            uint32_t len = 0;
            if (!PeekCodePoint(begin, end, ucc, len))
            {
                return false;
            }

            begin += len;
            return true;
        }

        [[nodiscard]] bool PeekCodePoint(uint32_t& ucc, uint32_t& len)
        {
            return PeekCodePoint(m_cursor, m_end, ucc, len);
        }

        [[nodiscard]] bool ConsumeCodePoint(uint32_t& ucc)
        {
            return ConsumeCodePoint(m_cursor, m_end, ucc);
        }

        [[nodiscard]] bool AtEnd() const { return m_cursor >= m_end || *m_cursor == '\0'; }

        [[nodiscard]] bool SkipSpaces(bool allowSlashdash = true)
        {
            uint32_t ucc = 0;
            uint32_t len = 0;
            while (!AtEnd() && PeekCodePoint(ucc, len))
            {
                // catch escape sequences for escaped newlines
                if (ucc == '\\')
                {
                    m_inWhitespaceEscape = true;
                    m_cursor += len;
                }
                // anywhere you can have whitespace, you can also have comments
                else if (ucc == '/')
                {
                    if (!PeekCodePoint(m_cursor + len, m_end, ucc, len))
                    {
                        return false;
                    }

                    // Single line comments and slashdash comments are terminators for nodes, they don't count just "spaces"
                    if (ucc == '/')
                    {
                        return true;
                    }

                    // Some contexts don't allow slashdash comments
                    if (!allowSlashdash && ucc == '-')
                    {
                        return true;
                    }

                    if (!ParseComment())
                    {
                        return false;
                    }
                }
                // if it is whitespace consume it and continue
                else if (IsKdlWhitespace(ucc))
                {
                    m_cursor += len;
                }
                // escaped newlines are considered normal spaces
                else if (IsKdlNewline(ucc) && m_inWhitespaceEscape)
                {
                    if (!ConsumeNewline())
                    {
                        return false;
                    }
                }
                // finally, if it isn't whitespace or comment we're done here
                else
                {
                    m_inWhitespaceEscape = false;
                    break;
                }
            }

            return true;
        }

        [[nodiscard]] bool ConsumeBOM()
        {
            if (static_cast<uint8_t>(*m_cursor) != 0xef)
            {
                return true;
            }

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbb) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidBom);
            }

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbf) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidBom);
            }

            ++m_cursor;
            return true;
        }

        [[nodiscard]] bool ConsumeVersion()
        {
            const char* begin = m_cursor;

            if (begin >= m_end || m_cursor[0] != '/'
                || begin + 1 >= m_end || m_cursor[1] != '-')
            {
                // Not a slashdash, so not a version marker
                return true;
            }

            begin += 2;

            uint32_t ucc = 0;
            uint32_t len = 0;

            // Skip past any unicode spaces before the marker
            while (begin < m_end && PeekCodePoint(begin, m_end, ucc, len))
            {
                if (!IsKdlWhitespace(ucc))
                {
                    break;
                }

                begin += len;
            }

            constexpr char VersionMarker[] = "kdl-version";
            if (begin + sizeof(VersionMarker) - 1 < m_end
                && MemEqual(begin, VersionMarker, sizeof(VersionMarker) - 1))
            {
                begin += sizeof(VersionMarker) - 1;
            }
            else
            {
                // This is a slashdash comment, not a version marker
                return true;
            }

            // Skip past any unicode spaces after the marker, there must be at least one
            bool hasWhitespace = false;
            while (begin < m_end && PeekCodePoint(begin, m_end, ucc, len))
            {
                if (!IsKdlWhitespace(ucc))
                {
                    break;
                }

                hasWhitespace = true;
                begin += len;
            }

            // This is a slashdash comment, not a version marker
            if (!hasWhitespace || begin >= m_end)
            {
                return true;
            }

            // Only version 2 is supported
            if (*begin != '2') [[unlikely]]
            {
                return SetError(KdlReadError::InvalidVersion, '2');
            }

            const StringView version = { begin, 1 };
            begin += 1;

            if (begin >= m_end) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof, '\n');
            }

            while (begin < m_end && PeekCodePoint(begin, m_end, ucc, len))
            {
                if (IsKdlWhitespace(ucc))
                {
                    begin += len;
                    continue;
                }

                if (IsKdlNewline(ucc))
                {
                    begin += len;
                    break;
                }

                return SetError(KdlReadError::InvalidToken, '\n');
            }

            m_cursor = begin;
            m_handler->Version(version);
            return true;
        }

        [[nodiscard]] bool Consume(char ch)
        {
            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            if (ucc != static_cast<uint32_t>(ch)) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidToken, ch);
            }

            m_cursor += len;
            return true;
        }

        [[nodiscard]] bool Consume(const char* str)
        {
            while (*str)
            {
                if (!Consume(*str))
                    return false;

                ++str;
            }
            return true;
        }

        [[nodiscard]] static bool IsOneOf(uint32_t ucc, const char* chars)
        {
            while (*chars)
            {
                if (ucc == static_cast<uint32_t>(*chars++))
                    return true;
            }

            return false;
        }

        [[nodiscard]] bool ConsumeNewline()
        {
            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            if (!IsKdlNewline(ucc)) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidToken, '\n');
            }

            m_cursor += len;

            // Special handling of CR which may be followed by LF
            if (ucc == '\r' && !AtEnd())
            {
                if (!PeekCodePoint(ucc, len))
                {
                    return false;
                }

                if (ucc == '\n') [[likely]]
                {
                    m_cursor += len;
                }
            }

            ++m_line;
            m_lineStart = m_cursor;
            m_inWhitespaceEscape = false;
            return true;
        }

        [[nodiscard]] bool IsEndOfString(const char* begin, const char* end, uint32_t rawDelimCount)
        {
            uint32_t count = 0;
            while (begin < end && *begin != '\0' && count < rawDelimCount)
            {
                uint32_t ucc = 0;
                if (!ConsumeCodePoint(begin, end, ucc))
                    return false;

                if (ucc != '#')
                    break;

                ++count;
            }

            return count == rawDelimCount;
        }

        [[nodiscard]] bool IsAnySlashdashOpen()
        {
            return !m_slashDashDepthStack.IsEmpty();
        }

        [[nodiscard]] bool IsSlashdashOpen()
        {
            return !m_slashDashDepthStack.IsEmpty() && m_slashDashDepthStack.Back() == m_nodeDepth;
        }

        [[nodiscard]] bool PushOpenSlashdash()
        {
            // Slashdash after a type is not allowed
            if (!m_typeBuffer.IsEmpty())
            {
                return SetError(KdlReadError::InvalidToken);
            }

            m_slashDashDepthStack.PushBack(m_nodeDepth);

            if (!m_handler->StartComment()) [[unlikely]]
            {
                return SetError(KdlReadError::Cancelled);
            }

            return true;
        }

        [[nodiscard]] bool PopOpenSlashdash()
        {
            if (!m_slashDashDepthStack.IsEmpty() && m_slashDashDepthStack.Back() == m_nodeDepth)
            {
                m_slashDashDepthStack.PopBack();

                if (!m_handler->EndComment()) [[unlikely]]
                {
                    return SetError(KdlReadError::Cancelled);
                }
            }

            return true;
        }

        [[nodiscard]] bool GetDedentPrefix(StringView& outDedentPrefix, uint32_t rawDelimCount)
        {
            const char* begin = m_cursor;
            const char* lineStart = begin;
            bool inEscapeSequence = false;
            while (begin < m_end && *begin != '\0')
            {
                uint32_t ucc = 0;
                if (!ConsumeCodePoint(begin, m_end, ucc))
                {
                    return false;
                }

                // Hitting any non-whitespace character while in an escape sequence will end it.
                // We're not parsing escape sequences, just searching for the end of the string.
                // The only reason we check for escaped whitespace is to ensure that `lineStart`
                // only changes when hitting an unescaped newline.
                if (inEscapeSequence)
                {
                    if (!IsKdlWhitespace(ucc) && !IsKdlNewline(ucc))
                    {
                        inEscapeSequence = false;
                    }
                }
                // If we're not in an escape sequence and we hit a backslash,
                // this is the start of a new escape sequence.
                else if (ucc == '\\')
                {
                    inEscapeSequence = true;
                }

                // Check for send of string and store the dedent prefix
                if (!inEscapeSequence)
                {
                    if (ucc == '"'
                        && begin + 0 < m_end && begin[0] == '"'
                        && begin + 1 < m_end && begin[1] == '"')
                    {
                        if (IsEndOfString(begin + 2, m_end, rawDelimCount))
                        {
                            outDedentPrefix = { lineStart, begin - 1 }; // -1 for the consumed quote
                            break;
                        }
                    }

                    if (IsKdlNewline(ucc))
                    {
                        lineStart = begin;
                    }
                }
            }

            // This shouldn't happen, but if it does, it's an error.
            if (inEscapeSequence) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidEscapeSequence);
            }

            // Validate that the final line of the string is only whitespace, or escaped whitespace
            for (const uint32_t ucc : UTF8Splitter(outDedentPrefix))
            {
                if (!inEscapeSequence && ucc == '\\')
                {
                    inEscapeSequence = true;
                    continue;
                }

                const bool allowed = IsKdlWhitespace(ucc) || (inEscapeSequence && IsKdlNewline(ucc));
                if (!allowed)
                {
                    return SetError(KdlReadError::InvalidToken, ' ');
                }
            }

            return true;
        }

        [[nodiscard]] bool ConsumeType(bool& hasType)
        {
            if (!Consume('('))
            {
                return false;
            }

            if (!SkipSpaces(false))
            {
                return false;
            }

            StringView type;
            if (!ConsumeString(type))
            {
                return false;
            }

            if (!SkipSpaces(false))
            {
                return false;
            }

            if (!Consume(')'))
            {
                return false;
            }

            if (!SkipSpaces(false))
            {
                return false;
            }

            hasType = true;
            m_typeBuffer = type;
            return true;
        }

        [[nodiscard]] bool ConsumeDedentPrefix(StringView dedentPrefix)
        {
            // Empty lines without a dedent prefix are allowed.
            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            if (IsKdlNewline(ucc))
                return true;

            for (const uint32_t prefixUcc : UTF8Splitter(dedentPrefix))
            {
                // We know at this point that the dedent prefix is only whitespace characters.
                // So we can assume if we see an backslash, it's a whitespace escape sequence
                // and we can skip the rest of the prefix.
                if (prefixUcc == '\\')
                {
                    break;
                }

                if (ucc != prefixUcc) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidToken, prefixUcc);
                }

                m_cursor += len;

                if (!PeekCodePoint(ucc, len))
                    return false;
            }

            return true;
        }

        [[nodiscard]] bool ConsumeUnicodeEscapeSequence(String& dst)
        {
            if (!Consume('u'))
            {
                return false;
            }

            if (!Consume('{')) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidEscapeSequence);
            }

            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            // Max of 6 hex characters
            bool foundNum = false;
            uint32_t ucc = 0;
            for (uint32_t i = 0; i < 6; ++i)
            {
                if (AtEnd()) [[unlikely]]
                {
                    return SetError(KdlReadError::UnexpectedEof);
                }

                if (*m_cursor == '}')
                {
                    break;
                }

                if (!IsHex(*m_cursor)) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidEscapeSequence);
                }

                ucc = (ucc << 4) + HexToNibble(*m_cursor);
                foundNum = true;
                ++m_cursor;
            }

            if (!Consume('}')) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidEscapeSequence);
            }

            if (!foundNum) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidEscapeSequence);
            }

            if (!IsKdlUnicodeScalarValue(ucc)) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidEscapeSequence);
            }

            const uint32_t len = UTF8Encode(dst, ucc);
            if (len == 0) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidEscapeSequence);
            }

            return true;
        }

        [[nodiscard]] bool ConsumeQuotedString(String& dst, uint32_t rawDelimCount)
        {
            dst.Clear();

            bool firstChar = true;
            bool isMultiline = false;
            bool inEscapeSeq = false;
            StringView dedentPrefix;
            while (!AtEnd()) [[likely]]
            {
                uint32_t ucc = 0;
                uint32_t len = 0;
                if (!PeekCodePoint(ucc, len))
                {
                    return false;
                }

                // Multi-line string handling
                if (firstChar)
                {
                    firstChar = false;

                    if (ucc == '"')
                    {
                        if (m_cursor + 1 < m_end && m_cursor[1] == '"')
                        {
                            isMultiline = true;
                        }
                    }

                    if (isMultiline)
                    {
                        // consume the two quotes
                        m_cursor += 2;

                        if (!ConsumeNewline())
                        {
                            return false;
                        }

                        // Multi-line strings dedent by the number of spaces on the last line
                        // of the string. This means we need to scan to the end of the string
                        // and store the whitespace prefix of the final line.
                        if (!GetDedentPrefix(dedentPrefix, rawDelimCount))
                        {
                            return false;
                        }

                        // TODO: Only consume the dedent prefix if the line has non-whitespace characters
                        // See: https://github.com/kdl-org/kdl/issues/503
                        if (!ConsumeDedentPrefix(dedentPrefix))
                        {
                            return false;
                        }

                        continue;
                    }
                }

                // Escape sequence handling
                if (inEscapeSeq)
                {
                    if (IsKdlWhitespace(ucc) || IsKdlNewline(ucc))
                    {
                        do
                        {
                            m_cursor += len;
                            if (!PeekCodePoint(ucc, len))
                            {
                                return false;
                            }
                        } while (IsKdlWhitespace(ucc) || IsKdlNewline(ucc));

                        inEscapeSeq = false;
                        continue;
                    }

                    if (ucc == 'u')
                    {
                        if (!ConsumeUnicodeEscapeSequence(dst))
                        {
                            return false;
                        }

                        inEscapeSeq = false;
                        continue;
                    }

                    switch (ucc)
                    {
                        case 'n': dst.Append('\n'); break;
                        case 'r': dst.Append('\r'); break;
                        case 't': dst.Append('\t'); break;
                        case '\\': dst.Append('\\'); break;
                        case '"': dst.Append('"'); break;
                        case 'b': dst.Append('\b'); break;
                        case 'f': dst.Append('\f'); break;
                        case 's': dst.Append(' '); break;
                        [[unlikely]] default:
                            return SetError(KdlReadError::InvalidEscapeSequence);
                    }

                    inEscapeSeq = false;
                    m_cursor += len;
                    continue;
                }

                // Newline handling is different for multi-line or single-line strings
                if (IsKdlNewline(ucc))
                {
                    // unescaped newlines are not allowed in single-line strings
                    if (!isMultiline) [[unlikely]]
                    {
                        return SetError(KdlReadError::InvalidControlChar);
                    }

                    if (!ConsumeNewline())
                    {
                        return false;
                    }

                    // Consume the required dedent prefix since we consumed a newline
                    if (!ConsumeDedentPrefix(dedentPrefix))
                    {
                        return false;
                    }

                    // newlines are normalized to `\n`
                    dst.PushBack('\n');
                    continue;
                }

                // New escape sequence start handling
                if (ucc == '\\')
                {
                    if (rawDelimCount > 0)
                    {
                        dst.PushBack('\\');
                    }
                    else
                    {
                        inEscapeSeq = true;
                    }
                    m_cursor += len;
                    continue;
                }

                // End of string handling
                if (ucc == '"')
                {
                    const bool isMultilineEnd = isMultiline
                        && m_cursor + 1 < m_end && m_cursor[1] == '"'
                        && m_cursor + 2 < m_end && m_cursor[2] == '"';

                    if (!isMultiline || isMultilineEnd)
                    {
                        const uint32_t multilineOffset = isMultilineEnd ? 2 : 0;
                        const char* strEnd = m_cursor + len + multilineOffset;
                        if (IsEndOfString(strEnd, m_end, rawDelimCount))
                        {
                            m_cursor = strEnd;
                            m_cursor += rawDelimCount;
                            if (!dst.IsEmpty() && isMultiline && dst.Back() == '\n')
                            {
                                dst.PopBack();
                            }
                            return true;
                        }
                    }
                }

                // Append the character to the buffer
                dst.Append(m_cursor, len);
                m_cursor += len;
            }

            return SetError(KdlReadError::UnexpectedEof);
        }

        [[nodiscard]] bool ConsumeIdentifierString(StringView& value)
        {
            const char* begin = m_cursor;

            bool first = true;
            while (!AtEnd())
            {
                uint32_t ucc = 0;
                uint32_t len = 0;
                if (!PeekCodePoint(ucc, len))
                {
                    return false;
                }

                if (first && !IsValidKdlIdentifierStartCodePoint(ucc)) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidIdentifier);
                }

                first = false;

                if (!IsValidKdlIdentifierCodePoint(ucc))
                {
                    break;
                }

                m_cursor += len;
            }

            value = { begin, m_cursor };

            // No identifier string can start with a number
            if (IsNumeric(value[0])) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidIdentifier);
            }

            if (value[0] == '-' || value[0] == '+')
            {
                // Identifiers can start with "-."/"+." as long as the next char is not a number
                if (value.Size() > 2 && value[1] == '.' && IsNumeric(value[2])) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidIdentifier);
                }

                // Identifiers can start with "-"/"+" as long as the next char is not a number
                if (value.Size() > 1 && IsNumeric(value[1])) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidIdentifier);
                }
            }

            if (value[0] == '.')
            {
                // Identifiers can start with "." as long as the next char is not a number
                if (value.Size() > 1 && IsNumeric(value[1])) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidIdentifier);
                }
            }

            // Identifiers can't be any of the language keywords without their leading '#'
            if (value == "inf" || value == "-inf" || value == "nan" || value == "true" || value == "false" || value == "null") [[unlikely]]
            {
                return SetError(KdlReadError::InvalidIdentifier);
            }

            return true;
        }

        [[nodiscard]] bool ConsumeString(StringView& value)
        {
            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            if (ucc == '"')
            {
                m_cursor += len;
                if (!ConsumeQuotedString(m_stringBuffer, 0))
                {
                    return false;
                }

                value = m_stringBuffer;
                return true;
            }

            if (ucc == '#')
            {
                m_cursor += len;
                uint32_t rawDelimCount = 1;
                while (!AtEnd())
                {
                    if (!ConsumeCodePoint(ucc))
                    {
                        return false;
                    }

                    if (ucc == '#')
                    {
                        ++rawDelimCount;
                        continue;
                    }

                    if (ucc == '"')
                    {
                        if (!ConsumeQuotedString(m_stringBuffer, rawDelimCount))
                        {
                            return false;
                        }

                        value = m_stringBuffer;
                        return true;
                    }
                }

                return SetError(KdlReadError::UnexpectedEof);
            }

            return ConsumeIdentifierString(value);
        }

    private:
        [[nodiscard]] bool ParseExpression()
        {
            if (!SkipSpaces())
            {
                return false;
            }

            if (AtEnd())
            {
                if (m_nodeDepth > 0) [[unlikely]]
                {
                    return SetError(KdlReadError::UnexpectedEof);
                }

                return true;
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            if (IsKdlNewline(ucc))
            {
                return ConsumeNewline();
            }

            switch (ucc)
            {
                case '/':
                {
                    return ParseComment();
                }
                case ';':
                {
                    // empty nodes are allowed, just skip the semicolon
                    m_cursor += len;
                    return true;
                }
                case '(':
                case '"':
                case '#':
                {
                    // start of type annotation or string name of node
                    return ParseNode();
                }
                default:
                {
                    if (IsValidKdlIdentifierStartCodePoint(ucc)) [[likely]]
                    {
                        return ParseNode();
                    }

                    return SetError(KdlReadError::InvalidToken);
                }
            }
        }

        [[nodiscard]] bool ParseComment()
        {
            if (!Consume('/'))
            {
                return false;
            }

            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            switch (ucc)
            {
                case '-':
                {
                    // slashdash
                    m_cursor += len;
                    return PushOpenSlashdash();
                }
                case '/':
                {
                    // single-line comment
                    m_cursor += len;
                    if (!SkipSpaces())
                    {
                        return false;
                    }

                    const char* begin = m_cursor;

                    // Scan forward until we find a newline
                    while (!AtEnd())
                    {
                        if (!PeekCodePoint(ucc, len))
                        {
                            return false;
                        }

                        if (IsKdlNewline(ucc))
                        {
                            break;
                        }

                        m_cursor += len;
                    }

                    const StringView trimmed = TrimKdlWhitespace({ begin, m_cursor });
                    if (!m_handler->Comment(trimmed)) [[unlikely]]
                    {
                        return SetError(KdlReadError::Cancelled);
                    }

                    if (!AtEnd())
                    {
                        if (!ConsumeNewline())
                        {
                            return false;
                        }
                    }

                    return true;
                }
                case '*':
                {
                    // multi-line comment
                    m_cursor += len;
                    if (!SkipSpaces())
                    {
                        return false;
                    }

                    m_stringBuffer.Clear();

                    uint32_t prevUcc = 0;
                    uint32_t depth = 1;
                    while (depth > 0)
                    {
                        if (AtEnd()) [[unlikely]]
                        {
                            return SetError(KdlReadError::UnexpectedEof);
                        }

                        if (!PeekCodePoint(ucc, len))
                        {
                            return false;
                        }

                        if (ucc == '*' && prevUcc == '/')
                        {
                            ++depth;
                            ucc = 0; // reset so "/*/" isn't self-closing
                        }
                        else if (ucc == '/' && prevUcc == '*')
                        {
                            --depth;
                            ucc = 0; // reset so "*/*" isn't self-opening
                        }

                        if (IsKdlNewline(ucc))
                        {
                            if (!ConsumeNewline())
                            {
                                return false;
                            }

                            // newlines are normalized to `\n`
                            m_stringBuffer.PushBack('\n');
                        }
                        else
                        {
                            m_stringBuffer.Append(m_cursor, len);
                            m_cursor += len;
                        }

                        prevUcc = ucc;
                    }

                    // remove the trailing "*/"
                    if (m_stringBuffer.Size() > 2)
                    {
                        m_stringBuffer.Resize(m_stringBuffer.Size() - 2);
                    }

                    const StringView trimmed = TrimKdlWhitespace(m_stringBuffer);
                    if (!m_handler->Comment(trimmed)) [[unlikely]]
                    {
                        return SetError(KdlReadError::Cancelled);
                    }

                    return true;
                }
                [[unlikely]] default:
                {
                    return SetError(KdlReadError::InvalidToken, '/');
                }
            }
        }

        [[nodiscard]] bool ParseNode()
        {
            if (!SkipSpaces())
            {
                return false;
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            bool hasType = false;
            if (ucc == '(')
            {
                if (!ConsumeType(hasType))
                {
                    return false;
                }
            }

            StringView name;
            if (!ConsumeString(name))
            {
                return false;
            }

            if (!EmitStartNode(name, hasType))
            {
                return false;
            }

            if (AtEnd())
            {
                return EmitEndNode();
            }

            // Next must be: whitespace, newline, semicolon, single-line comment, or brace
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            if (!IsKdlWhitespace(ucc) && !IsKdlNewline(ucc) && ucc != ';' && ucc != '/' && ucc != '{' && ucc != '}') [[unlikely]]
            {
                return SetError(KdlReadError::InvalidToken);
            }

            bool hasWhitespace = false;
            bool hasAnyPropOrArg = false;
            bool hasAnyChildBlock = false;
            bool hasActiveChildBlock = false;
            while (!AtEnd())
            {
                if (!PeekCodePoint(ucc, len))
                {
                    return false;
                }

                // Newline is a node terminator if no slashdash is active.
                if (IsKdlNewline(ucc))
                {
                    if (!ConsumeNewline())
                    {
                        return false;
                    }

                    if (!IsSlashdashOpen())
                    {
                        return EmitEndNode();
                    }

                    continue;
                }

                // Whitespace is required between the node name and the first argument or property
                if (IsKdlWhitespace(ucc))
                {
                    hasWhitespace = true;
                    if (!SkipSpaces())
                    {
                        return false;
                    }

                    continue;
                }

                // consume an argument, property, or children
                switch (ucc)
                {
                    // Single-line comment is a node terminator, end the node and then consume it.
                    // Multi-line and slashdash comments are allowed in the node
                    case '/':
                    {
                        if (!IsSlashdashOpen() && m_cursor + 1 < m_end && m_cursor[1] == '/' && !m_inWhitespaceEscape)
                        {
                            if (!EmitEndNode())
                            {
                                return false;
                            }

                            return ParseComment();
                        }

                        if (!ParseComment())
                        {
                            return false;
                        }

                        break;
                    }
                    // Semicolon is a node terminator, end the node and then consume it.
                    case ';':
                    {
                        if (!EmitEndNode())
                        {
                            return false;
                        }

                        m_cursor += len;
                        return true;
                    }
                    // End brace is a node terminator, end the node but don't consume it.
                    // Let ParseNodeChildBlock consume it when handling the child block.
                    case '}':
                    {
                        if (m_nodeDepth == 0) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken);
                        }

                        if (!EmitEndNode())
                        {
                            return false;
                        }

                        return true;
                    }
                    // Open brace indicates we're starting a child block.
                    case '{':
                    {
                        if (hasAnyPropOrArg && !hasWhitespace) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken, ' ');
                        }

                        if (hasActiveChildBlock)
                        {
                            if (!IsSlashdashOpen()) [[unlikely]]
                            {
                                return SetError(KdlReadError::InvalidToken);
                            }
                        }
                        else
                        {
                            hasActiveChildBlock = !IsSlashdashOpen();
                        }

                        hasAnyChildBlock = true;
                        m_cursor += len;

                        if (!ParseNodeChildBlock())
                        {
                            return false;
                        }

                        break;
                    }
                    // either a keyword value, or start of a raw string
                    case '#':
                    // a type annotation here can only be for an argument value
                    case '(':
                    // a number here can only be an argument value
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    // start of a string could be an argument value or property name
                    case '"':
                    {
                        if (!IsSlashdashOpen() && !hasWhitespace) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken, ' ');
                        }

                        // Arguments & properties must come before child blocks, even commented out ones
                        if (hasAnyChildBlock) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken);
                        }

                        if (!ParseArgumentOrProperty())
                        {
                            return false;
                        }

                        hasWhitespace = false;
                        break;
                    }
                    // anything else must be an identifier string
                    // which could be an argument value or property name
                    default:
                    {
                        if (!IsSlashdashOpen() && !hasWhitespace) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken, ' ');
                        }

                        if (!IsValidKdlIdentifierStartCodePoint(ucc)) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken);
                        }

                        // Arguments & properties must come before child blocks, even commented out ones
                        if (hasAnyChildBlock) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken);
                        }

                        if (!ParseArgumentOrProperty())
                        {
                            return false;
                        }

                        hasAnyPropOrArg = true;
                        hasWhitespace = false;
                        break;
                    }
                }
            }

            // End the node if we reach EOF
            if (!EmitEndNode())
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool ParseNodeChildBlock()
        {
            // Extra depth for child blocks ensures that we don't confuse a slashdash
            // on a child node with the slashdash on the child node block.
            // This will be decremented when the child block ends.
            ++m_nodeDepth;

            const uint32_t depth = m_nodeDepth;
            while (m_nodeDepth >= depth)
            {
                if (!SkipSpaces())
                {
                    return false;
                }

                if (AtEnd()) [[unlikely]]
                {
                    return SetError(KdlReadError::UnexpectedEof);
                }

                uint32_t ucc = 0;
                uint32_t len = 0;
                if (!PeekCodePoint(ucc, len))
                {
                    return false;
                }

                // End the child block
                if (ucc == '}')
                {
                    m_cursor += len;
                    break;
                }

                if (!ParseExpression())
                {
                    return false;
                }
            }

            if (m_nodeDepth == 0) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidToken);
            }

            // Can't have a dangling slashdash when a node ends a child block
            if (IsSlashdashOpen())
            {
                return SetError(KdlReadError::InvalidToken);
            }

            // End the child block's extra depth
            --m_nodeDepth;

            // Complete any slashdash for the child block
            return PopOpenSlashdash();
        }

        [[nodiscard]] bool EmitStartNode(StringView name, bool hasType)
        {
            StringView typeViewStorage = hasType ? m_typeBuffer : "";
            const StringView* typeView = hasType ? &typeViewStorage : nullptr;

            if (!m_handler->StartNode(name, typeView)) [[unlikely]]
            {
                return SetError(KdlReadError::Cancelled);
            }

            ++m_nodeDepth;

            if (m_nodeDepth > m_maxNodeDepth) [[unlikely]]
            {
                return SetError(KdlReadError::MaxDepthExceeded);
            }

            m_typeBuffer.Clear();
            return true;
        }

        [[nodiscard]] bool EmitEndNode()
        {
            if (m_nodeDepth == 0) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidToken);
            }

            // Can't have a dangling slashdash when a node ends
            if (IsSlashdashOpen())
            {
                return SetError(KdlReadError::InvalidToken);
            }

            if (!m_handler->EndNode()) [[unlikely]]
            {
                return SetError(KdlReadError::Cancelled);
            }

            --m_nodeDepth;

            // Complete any slashdash for the node itself
            return PopOpenSlashdash();
        }

        template <typename T>
        [[nodiscard]] bool EmitPropOrArg(T value, bool hasType, const StringView* propName)
        {
            StringView typeViewStorage = hasType ? m_typeBuffer : "";
            const StringView* typeView = hasType ? &typeViewStorage : nullptr;

            if (propName)
            {
                if (!m_handler->Property(*propName, value, typeView)) [[unlikely]]
                {
                    return SetError(KdlReadError::Cancelled);
                }
            }
            else
            {
                if (!m_handler->Argument(value, typeView)) [[unlikely]]
                {
                    return SetError(KdlReadError::Cancelled);
                }
            }

            m_typeBuffer.Clear();

            // Complete any slashdash for the prop or arg
            return PopOpenSlashdash();
        }

        [[nodiscard]] bool ParseIntFromBuffer(uint32_t base, bool hasType, const StringView* propName)
        {
            if (m_stringBuffer.IsEmpty()) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidNumber);
            }

            if (m_stringBuffer[0] == '-')
            {
                int64_t value = 0;
                const char* end = m_stringBuffer.End();
                if (!StrToInt(value, m_stringBuffer.Begin(), &end, base)) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidNumber);
                }

                return EmitPropOrArg(value, hasType, propName);
            }

            uint64_t value = 0;
            const char* end = m_stringBuffer.End();
            if (!StrToInt(value, m_stringBuffer.Begin(), &end, base)) [[unlikely]]
            {
                return SetError(KdlReadError::InvalidNumber);
            }

            return EmitPropOrArg(value, hasType, propName);
        }

        [[nodiscard]] bool ParseHexNum(bool hasType, const StringView* propName)
        {
            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            if (*m_cursor == '_') [[unlikely]]
            {
                return SetError(KdlReadError::InvalidNumber);
            }

            while (!AtEnd() && (IsHex(*m_cursor) || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                {
                    m_stringBuffer.PushBack(*m_cursor);
                }
                ++m_cursor;
            }

            return ParseIntFromBuffer(16, hasType, propName);
        }

        [[nodiscard]] bool ParseOctNum(bool hasType, const StringView* propName)
        {
            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            if (*m_cursor == '_') [[unlikely]]
            {
                return SetError(KdlReadError::InvalidNumber);
            }

            while (!AtEnd() && ((*m_cursor >= '0' && *m_cursor <= '7') || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                {
                    m_stringBuffer.PushBack(*m_cursor);
                }
                ++m_cursor;
            }

            return ParseIntFromBuffer(8, hasType, propName);
        }

        [[nodiscard]] bool ParseBinNum(bool hasType, const StringView* propName)
        {
            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            if (*m_cursor == '_') [[unlikely]]
            {
                return SetError(KdlReadError::InvalidNumber);
            }

            while (!AtEnd() && (*m_cursor == '0' || *m_cursor == '1' || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                {
                    m_stringBuffer.PushBack(*m_cursor);
                }
                ++m_cursor;
            }

            return ParseIntFromBuffer(2, hasType, propName);
        }

        [[nodiscard]] bool ParseDecNum(bool hasType, const StringView* propName)
        {
            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            const char* dot = nullptr;
            const char* exp = nullptr;
            const char* expSign = nullptr;
            bool prevWasNumeric = false;

            while (!AtEnd())
            {
                uint32_t ucc = 0;
                uint32_t len = 0;
                if (!PeekCodePoint(ucc, len))
                {
                    return false;
                }

                if ((ucc < '0' || ucc > '9') && !IsOneOf(ucc, ".eE-+_"))
                {
                    break;
                }

                switch (ucc)
                {
                    case '.':
                    {
                        if (!prevWasNumeric) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidNumber);
                        }

                        if (dot) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidNumber);
                        }

                        dot = m_cursor;
                        break;
                    }
                    case 'e':
                    case 'E':
                    {
                        if (!prevWasNumeric) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidNumber);
                        }

                        if (exp) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidNumber);
                        }

                        exp = m_cursor;
                        break;
                    }
                    case '+':
                    case '-':
                    {
                        if (!exp || expSign) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidNumber);
                        }

                        expSign = m_cursor;
                        break;
                    }
                    case '_':
                    {
                        if (dot == (m_cursor - len)) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidNumber);
                        }

                        break;
                    }
                }

                if (ucc != '_')
                {
                    m_stringBuffer.Append(m_cursor, len);
                }

                prevWasNumeric = ucc >= '0' && ucc <= '9';
                m_cursor += len;
            }

            if (dot || exp)
            {
                if (m_stringBuffer.IsEmpty()) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidNumber);
                }

                const char* begin = m_stringBuffer.Begin();
                if (*begin == '-')
                {
                    ++begin;
                }

                const char* end = m_stringBuffer.End();

                // Cannot start with a dot or exponent
                if (*begin == '.' || *begin == 'e' || *begin == 'E') [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidNumber);
                }

                // Cannot end with a dot, exponent, or sign
                if (end[-1] == '.' || end[-1] == 'e' || end[-1] == 'E' || end[-1] == '-' || end[-1] == '+') [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidNumber);
                }

                // Exponent cannot be before the dot
                if (exp && exp < dot) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidNumber);
                }

                double value = 0.0;
                if (!StrToFloat(value, m_stringBuffer.Begin(), &end)) [[unlikely]]
                {
                    return SetError(KdlReadError::InvalidNumber);
                }

                return EmitPropOrArg(value, hasType, propName);
            }

            return ParseIntFromBuffer(10, hasType, propName);
        }

        [[nodiscard]] bool ParseNum(bool hasType, const StringView* propName = nullptr)
        {
            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            m_stringBuffer.Clear();

            if (*m_cursor == '-')
            {
                if (!Consume('-'))
                {
                    return false;
                }

                m_stringBuffer.PushBack('-');
            }
            else if (*m_cursor == '+')
            {
                if (!Consume('+'))
                {
                    return false;
                }
            }

            if (AtEnd()) [[unlikely]]
            {
                return SetError(KdlReadError::UnexpectedEof);
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            // 0x, 0o, 0b, 0, 0.0
            if (ucc == '0')
            {
                m_cursor += len;

                if (!AtEnd())
                {
                    switch (*m_cursor)
                    {
                        case 'x': return Consume('x') && ParseHexNum(hasType, propName);
                        case 'o': return Consume('o') && ParseOctNum(hasType, propName);
                        case 'b': return Consume('b') && ParseBinNum(hasType, propName);
                    }
                }

                // Back up the cursor so the leading zero is restored, and fall through to
                // decimal number parsing.
                m_cursor -= len;
            }

            return ParseDecNum(hasType, propName);
        }

        [[nodiscard]] bool ParseArgumentOrProperty(const StringView* propName = nullptr)
        {
            if (!SkipSpaces(!propName))
            {
                return false;
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
            {
                return false;
            }

            // Consume the type annotation if present
            bool hasType = false;
            if (ucc == '(')
            {
                if (!ConsumeType(hasType))
                {
                    return false;
                }

                if (!PeekCodePoint(ucc, len))
                {
                    return false;
                }
            }

            switch (ucc)
            {
                // keyword value, or raw string
                // #true, #false, #null, #nan, #inf, #-inf, #"rawstr"#
                case '#':
                {
                    // If the next character is a quote, or another hash, then this is a raw string
                    if (m_cursor + 1 < m_end && (m_cursor[1] == '"' || m_cursor[1] == '#'))
                    {
                        StringView value;
                        if (!ConsumeString(value))
                        {
                            return false;
                        }

                        return EmitPropOrArg(value, hasType, propName);
                    }

                    // otherwise, this is a keyword value
                    m_cursor += len;
                    switch (*m_cursor)
                    {
                        case 't': return Consume("true") ? EmitPropOrArg(true, hasType, propName) : false;
                        case 'f': return Consume("false") ? EmitPropOrArg(false, hasType, propName) : false;
                        case 'i': return Consume("inf") ? EmitPropOrArg(Limits<double>::Infinity, hasType, propName) : false;
                        case '-': return Consume("-inf") ? EmitPropOrArg(-Limits<double>::Infinity, hasType, propName) : false;
                        case 'n':
                        {
                            if (m_cursor + 1 < m_end && m_cursor[1] == 'a')
                            {
                                return Consume("nan") ? EmitPropOrArg(Limits<double>::NaN, hasType, propName) : false;
                            }

                            return Consume("null") ? EmitPropOrArg(nullptr, hasType, propName) : false;
                        }
                    }

                    return SetError(KdlReadError::InvalidToken);
                }
                // any digit means this is a number
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                {
                    return ParseNum(hasType, propName);
                }
                // could be a number or the start of an identifier string
                case '-':
                case '+':
                {
                    if (m_cursor + len < m_end && m_cursor[len] != '\0' && IsNumeric(m_cursor[len]))
                    {
                        return ParseNum(hasType, propName);
                    }

                    // fall through to string parsing
                    [[fallthrough]];
                }
                // anything else must be a string
                default:
                {
                    StringView value;
                    if (!ConsumeString(value))
                    {
                        return false;
                    }

                    if (AtEnd())
                    {
                        return EmitPropOrArg(value, hasType, propName);
                    }

                    const char* beforeSpaces = m_cursor;
                    if (!SkipSpaces(false))
                    {
                        return false;
                    }

                    if (AtEnd())
                    {
                        return EmitPropOrArg(value, hasType, propName);
                    }

                    if (!PeekCodePoint(ucc, len))
                    {
                        return false;
                    }

                    if (IsKdlEqualsSign(ucc))
                    {
                        if (propName) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken);
                        }

                        if (hasType) [[unlikely]]
                        {
                            return SetError(KdlReadError::InvalidToken);
                        }

                        m_cursor += len;
                        return ParseArgumentOrProperty(&value);
                    }

                    // If not an equal sign, then we need to back up to before we tried to
                    // parse this as a property. Otherwise the required spaces between args
                    // and props will get consumed here.
                    m_cursor = beforeSpaces;
                    return EmitPropOrArg(value, hasType, propName);
                }
            }
        }

    private:
        KdlReadResult m_result{};
        KdlReader::Handler* m_handler{ nullptr };

        const char* m_cursor{ nullptr };
        const char* m_end{ nullptr };
        const char* m_lineStart{ nullptr };
        uint32_t m_line{ 1 };
        uint16_t m_nodeDepth{ 0 };
        uint16_t m_maxNodeDepth{ 8192 };
        bool m_inWhitespaceEscape{ false };

        String m_stringBuffer;
        String m_typeBuffer;
        Vector<uint16_t> m_slashDashDepthStack;
    };

    // --------------------------------------------------------------------------------------------
    KdlReader::KdlReader(Allocator& allocator) noexcept
        : m_allocator(allocator)
    {}

    KdlReadResult KdlReader::Read(StringView data, Handler& handler)
    {
        KdlParser parser(m_allocator);
        return parser.Parse(data, handler);
    }

    // --------------------------------------------------------------------------------------------
    template <>
    const char* EnumTraits<KdlReadError>::ToString(KdlReadError x) noexcept
    {
        switch (x)
        {
            case KdlReadError::None: return "None";
            case KdlReadError::Cancelled: return "Cancelled";
            case KdlReadError::UnexpectedEof: return "UnexpectedEof";
            case KdlReadError::DisallowedUtf8: return "DisallowedUtf8";
            case KdlReadError::InvalidBom: return "InvalidBom";
            case KdlReadError::InvalidVersion: return "InvalidVersion";
            case KdlReadError::InvalidUtf8: return "InvalidUtf8";
            case KdlReadError::InvalidEscapeSequence: return "InvalidEscapeSequence";
            case KdlReadError::InvalidControlChar: return "InvalidControlChar";
            case KdlReadError::InvalidIdentifier: return "InvalidIdentifier";
            case KdlReadError::InvalidNumber: return "InvalidNumber";
            case KdlReadError::InvalidToken: return "InvalidToken";
            case KdlReadError::MaxDepthExceeded: return "MaxDepthExceeded";
        }

        return "<unknown>";
    }
}
