// Copyright Chad Engler

#include "he/core/kdl_reader.h"

#include "kdl_internal.h"

#include "he/core/enum_ops.h"
#include "he/core/limits.h"
#include "he/core/string.h"
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
            m_stringBuffer.Clear();

            if (!SkipBOM())
            {
                SetError(KdlReadError::InvalidBom);
                return m_result;
            }

            m_handler->StartDocument();

            while (!AtEnd())
            {
                if (!ParseExpression())
                    return m_result;
            }

            m_handler->EndDocument();

            return m_result;
        }

    private:
        bool SetError(KdlReadError error, char expected = '\0')
        {
            const uint32_t column = UTF8Length(m_lineStart, static_cast<uint32_t>(m_cursor - m_lineStart)) + 1;
            m_result = { error, m_line, column, expected };
            return false;
        }

        [[nodiscard]] bool PeekCodePoint(const char* begin, const char* end, uint32_t& ucc, uint32_t& len)
        {
            if (begin >= end || *begin == '\0')
                return SetError(KdlReadError::UnexpectedEof);

            len = UTF8Decode(ucc, begin, static_cast<uint32_t>(end - begin));

            // This means that the sequence was valid, but was missing trailing bytes required to
            // properly decode. If we were reading from a stream this would mean we need to read
            // more bytes. Since we're operating on a buffered string, this is an error.
            if (len == 0)
                return SetError(KdlReadError::UnexpectedEof);

            // This means that the sequence was invalid and could not be decoded.
            if (len == InvalidCodePoint)
                return SetError(KdlReadError::InvalidUtf8);

            // Some code points are not allowed in KDL documents.
            if (IsDisallowedKdlCodePoint(ucc))
                return SetError(KdlReadError::DisallowedUtf8);

            return true;
        }

        [[nodiscard]] bool ConsumeCodePoint(const char*& begin, const char* end, uint32_t& ucc)
        {
            uint32_t len = 0;
            if (!PeekCodePoint(begin, end, ucc, len))
                return false;

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

        [[nodiscard]] bool SkipBOM()
        {
            if (static_cast<uint8_t>(*m_cursor) != 0xef)
                return true;

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbb)
                return SetError(KdlReadError::InvalidBom);

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbf)
                return SetError(KdlReadError::InvalidBom);

            ++m_cursor;
            return true;
        }

        [[nodiscard]] bool SkipSpaces()
        {
            uint32_t ucc = 0;
            uint32_t len = 0;
            while (!AtEnd() && PeekCodePoint(ucc, len))
            {
                // escaped newlines are considered normal spaces
                if (ucc = '\\')
                {
                    m_cursor += len;
                    if (!ConsumeNewLine())
                        return false;
                }
                // anywhere you can have whitespace, you can also have comments
                else if (ucc == '/')
                {
                    // skip comments
                    if (!ParseComment())
                        return false;
                }
                // finally, if it isn't whitespace we're done here
                else if (!IsKdlWhitespace(ucc))
                {
                    break;
                }

                m_cursor += len;
            }

            return true;
        }

        [[nodiscard]] bool Consume(char ch)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            if (ucc != ch)
                return SetError(KdlReadError::InvalidToken, ch);

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
                if (ucc == *chars++)
                    return true;
            }

            return false;
        }

        [[nodiscard]] bool ConsumeOneOf(const char* chars)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            if (IsOneOf(ucc, chars))
            {
                m_cursor += len;
                return true;
            }

            return SetError(KdlReadError::InvalidToken);
        }

        [[nodiscard]] bool ConsumeNewLine()
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            if (!IsKdlNewline(ucc))
                return SetError(KdlReadError::InvalidToken, '\n');

            m_cursor += len;

            // Special handling of CR which may be followed by LF
            if (ucc == '\r' && !AtEnd())
            {
                if (!PeekCodePoint(ucc, len))
                    return false;

                if (ucc == '\n')
                    m_cursor += len;
            }

            ++m_line;
            m_lineStart = m_cursor;
            return true;
        }

        [[nodiscard]] bool IsEndOfRawString(const char* begin, const char* end, uint32_t rawDelimCount)
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

        [[nodiscard]] bool GetDedentPrefix(StringView& outDedentPrefix, uint32_t rawDelimCount)
        {
            const char* begin = m_cursor;
            const char* lineStart = begin;
            while (begin < m_end && *begin != '\0')
            {
                uint32_t ucc = 0;
                if (!ConsumeCodePoint(begin, m_end, ucc))
                    return false;

                if (ucc != '"')
                {
                    if (IsKdlNewline(ucc))
                    {
                        lineStart = begin;
                    }
                    continue;
                }

                // For a raw string we also need to check the number of delimiters
                if (!IsEndOfRawString(begin, m_end, rawDelimCount))
                    continue;

                // Store the dedent prefix
                outDedentPrefix = { lineStart, begin };

                // Validate that the final line of the string is only whitespace
                while (lineStart < begin)
                {
                    if (!ConsumeCodePoint(lineStart, begin, ucc))
                        return false;

                    if (!IsKdlWhitespace(ucc))
                        return SetError(KdlReadError::InvalidToken, ' ');
                }

                return true;
            }

            return SetError(KdlReadError::UnexpectedEof);
        }

        [[nodiscard]] bool ConsumeDedentPrefix(StringView dedentPrefix)
        {
            for (const uint32_t expectedUcc : Utf8Splitter(dedentPrefix))
            {
                uint32_t ucc = 0;
                if (!ConsumeCodePoint(ucc))
                    return false;

                if (ucc != expectedUcc)
                    return SetError(KdlReadError::InvalidToken, expectedUcc);
            }

            return true;
        }

        [[nodiscard]] bool ConsumeUnicodeEscapeSequence(String& dst)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            // Max of 6 hex characters
            bool foundOne = false;
            uint32_t ucc = 0;
            for (uint32_t i = 0; i < 6; ++i)
            {
                if (AtEnd())
                    break;

                const char ch = *m_cursor++;

                if (!IsHex(ch))
                    break;

                foundOne = true;
                ucc = (ucc << 4) + HexToNibble(ch);
            }

            if (!foundOne)
                return SetError(KdlReadError::InvalidEscapeSequence);

            if (ucc < 0x80)
            {
                dst.PushBack(static_cast<uint8_t>(ucc));
            }
            else if (ucc < 0x800)
            {
                dst.PushBack(static_cast<uint8_t>(0xc0 | (ucc >> 6)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (ucc & 0x3f)));
            }
            else if (ucc < 0x10000)
            {
                dst.PushBack(static_cast<uint8_t>(0xe0 | (ucc >> 12)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((ucc >> 6) & 0x3f)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (ucc & 0x3f)));
            }
            else
            {
                dst.PushBack(static_cast<uint8_t>(0xf0 | (ucc >> 18)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((ucc >> 12) & 0x3f)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((ucc >> 6) & 0x3f)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (ucc & 0x3f)));
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
            while (!AtEnd())
            {
                const char* begin = m_cursor;

                uint32_t ucc = 0;
                if (!ConsumeCodePoint(ucc))
                    return false;

                // Multi-line string handling
                if (firstChar)
                {
                    isMultiline = IsKdlNewline(ucc);
                    firstChar = false;

                    if (isMultiline)
                    {
                        // Multi-line strings dedent by the number of spaces on the last line
                        // of the string. This means we need to scan to the end of the string
                        // and store the whitespace prefix of the final line.
                        if (!GetDedentPrefix(dedentPrefix, rawDelimCount))
                            return false;

                        // Consume the first prefix since we consumed the newline
                        if (!ConsumeDedentPrefix(dedentPrefix))
                            return false;

                        continue;
                    }
                }

                // Escape sequence handling
                if (inEscapeSeq)
                {
                    if (IsKdlWhitespace(ucc) || IsKdlNewline(ucc))
                    {
                        // skip newlines and whitespace after escape sequence
                        continue;
                    }

                    switch (ucc)
                    {
                        case 'r': dst.Append('\r'); break;
                        case 'n': dst.Append('\n'); break;
                        case 't': dst.Append('\t'); break;
                        case '\\': dst.Append('\\'); break;
                        case '"': dst.Append('"'); break;
                        case 'b': dst.Append('\b'); break;
                        case 'f': dst.Append('\f'); break;
                        case 'u':
                            if (!ConsumeUnicodeEscapeSequence(dst))
                                return false;
                            break;
                            break;
                        default:
                            return SetError(KdlReadError::InvalidEscapeSequence);
                    }

                    inEscapeSeq = false;
                    continue;
                }

                // Newline handling is different for multi-line or single-line strings
                if (IsKdlNewline(ucc))
                {
                    if (isMultiline)
                    {
                        // newlines are normalized to `\n`
                        dst.PushBack('\n');

                        // Consume the required dedent prefix since we consumed a newline
                        if (!ConsumeDedentPrefix(dedentPrefix))
                            return false;
                    }
                    else
                    {
                        // unescaped newlines are not allowed in single-line strings
                        return SetError(KdlReadError::InvalidToken);
                    }
                    continue;
                }

                // New escape sequence start handling
                if (ucc == '\\')
                {
                    if (rawDelimCount > 0)
                        dst.PushBack('\\');
                    else
                        inEscapeSeq = true;
                    continue;
                }

                // End of string handling
                if (ucc == '"')
                {
                    if (IsEndOfRawString(m_cursor, m_end, rawDelimCount))
                    {
                        m_cursor += rawDelimCount;
                        return true;
                    }
                }

                // Append the character to the buffer
                dst.Append(begin, m_cursor);
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
                if (!ConsumeCodePoint(ucc))
                    return false;

                if (first && !IsValidKdlIdentifierStartCodePoint(ucc))
                    return SetError(KdlReadError::InvalidIdentifier);

                first = false;

                if (IsKdlWhitespace(ucc) || IsKdlNewline(ucc))
                    break;

                if (!IsValidKdlIdentifierCodePoint(ucc))
                    return SetError(KdlReadError::InvalidIdentifier);
            }

            value = { begin, m_cursor };

            // No identifier string can start with a number
            if (IsNumeric(value[0]))
                return SetError(KdlReadError::InvalidIdentifier);

            if (value[0] == '-')
            {
                // Identifiers can start with "-." as long as the next char is not a number
                if (value.Size() > 2 && value[1] == '.' && IsNumeric(value[2]))
                    return SetError(KdlReadError::InvalidIdentifier);

                // Identifiers can start with "-" as long as the next char is not a number
                if (value.Size() > 1 && IsNumeric(value[1]))
                    return SetError(KdlReadError::InvalidIdentifier);
            }

            if (value[0] == '.')
            {
                // Identifiers can start with "." as long as the next char is not a number
                if (value.Size() > 1 && IsNumeric(value[1]))
                    return SetError(KdlReadError::InvalidIdentifier);
            }

            // Identifiers can't be any of the language keywords without their leading '#'
            if (value == "inf" || value == "-inf" || value == "nan" || value == "true" || value == "false" || value == "null")
                return SetError(KdlReadError::InvalidIdentifier);

            return true;
        }

        [[nodiscard]] bool ConsumeString(StringView& value)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            if (ucc == '"')
            {
                m_cursor += len;
                if (!ConsumeQuotedString(m_stringBuffer, 0))
                    return false;

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
                        return false;

                    if (ucc == '#')
                    {
                        ++rawDelimCount;
                        continue;
                    }

                    if (ucc == '"')
                    {
                        if (!ConsumeQuotedString(m_stringBuffer, rawDelimCount))
                            return false;

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
                return false;

            if (AtEnd())
            {
                if (m_nodeDepth > 0)
                    return SetError(KdlReadError::UnexpectedEof);

                return true;
            }

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            if (IsKdlNewline(ucc))
                return ConsumeNewLine();

            switch (ucc)
            {
                case ';':
                {
                    // empty nodes are allowed, just skip the semicolon
                    m_cursor += len;
                    return true;
                }
                case '}':
                {
                    // end of a node child block
                    if (m_nodeDepth == 0)
                        return SetError(KdlReadError::InvalidDocument);

                    if (!m_handler->EndNode())
                        return SetError(KdlReadError::Cancelled);

                    --m_nodeDepth;

                    if (!CheckSlashdashEnd())
                        return false;

                    m_cursor += len;
                    return true;
                }
                case '(':
                case '"':
                    // start of type annotation or string name of node
                    return ParseNode();
                default:
                    if (IsValidKdlIdentifierStartCodePoint(ucc))
                        return ParseNode();

                    return SetError(KdlReadError::InvalidToken);
            }
        }

        [[nodiscard]] bool CheckSlashdashEnd()
        {
            if (m_slashDashDepth > 0 && m_slashDashDepth == m_nodeDepth + 1)
            {
                m_slashDashDepth = 0;
                if (!m_handler->EndComment())
                    return SetError(KdlReadError::Cancelled);
            }

            return true;
        }

        [[nodiscard]] bool ParseComment()
        {
            if (!Consume('/'))
                return false;

            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            switch (ucc)
            {
                case '-':
                {
                    // slashdash
                    m_cursor += len;
                    m_slashDashDepth = m_nodeDepth + 1;

                    if (!m_handler->StartComment())
                        return SetError(KdlReadError::Cancelled);

                    return true;
                }
                case '/':
                {
                    // single-line comment
                    m_cursor += len;
                    if (!SkipSpaces())
                        return false;
                    const char* begin = m_cursor;

                    // Scan forward until we find a newline
                    while (!AtEnd())
                    {
                        uint32_t ucc = 0;
                        uint32_t len = 0;
                        if (!PeekCodePoint(ucc, len))
                            return false;

                        if (IsKdlNewline(ucc))
                            break;

                        m_cursor += len;
                    }

                    if (!m_handler->Comment({ begin, m_cursor }))
                        return SetError(KdlReadError::Cancelled);

                    if (!ConsumeNewLine())
                        return false;

                    return true;
                }
                case '*':
                {
                    // multi-line comment
                    m_cursor += len;
                    if (!SkipSpaces())
                        return false;
                    const char* begin = m_cursor;

                    uint32_t prevUcc = 0;
                    uint32_t depth = 1;
                    while (depth > 0)
                    {
                        if (AtEnd())
                            return SetError(KdlReadError::UnexpectedEof);

                        uint32_t ucc = 0;
                        if (!ConsumeCodePoint(ucc))
                            return false;

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

                        prevUcc = ucc;
                    }

                    if (!m_handler->Comment({ begin, m_cursor }))
                        return SetError(KdlReadError::Cancelled);

                    return true;
                }
            }

            return SetError(KdlReadError::InvalidToken, '/');
        }

        [[nodiscard]] bool ParseNode()
        {
            if (!SkipSpaces())
                return false;

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            StringView type;
            if (ucc == '(')
            {
                if (!ConsumeType(type))
                    return false;

                if (!SkipSpaces())
                    return false;
            }

            StringView name;
            if (!ConsumeString(name))
                return false;

            if (!m_handler->StartNode(name, type))
                return SetError(KdlReadError::Cancelled);

            if (AtEnd())
            {
                if (!m_handler->EndNode())
                    return SetError(KdlReadError::Cancelled);

                if (!CheckSlashdashEnd())
                    return false;

                return true;
            }

            // Next must be: whitespace, newline, semicolon, single-line comment
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            if (!IsKdlWhitespace(ucc) && !IsKdlNewline(ucc) && ucc != ';' && ucc != '/')
                return SetError(KdlReadError::InvalidToken);

            bool hasWhitespace = false;
            while (!AtEnd())
            {
                if (!PeekCodePoint(ucc, len))
                    return false;

                // Newline is a node terminator, end the node and then consume it.
                if (IsKdlNewline(ucc))
                {
                    if (!m_handler->EndNode())
                        return SetError(KdlReadError::Cancelled);

                    if (!CheckSlashdashEnd())
                        return false;

                    return ConsumeNewLine();
                }

                // Whitespace is required between the node name and the first argument or property
                if (IsKdlWhitespace(ucc))
                {
                    hasWhitespace = true;
                    m_cursor += len;
                    continue;
                }

                // consume an argument, property, or start of children
                switch (ucc)
                {
                    // single-line comment is a node terminator, end the node and then consume it.
                    // multi-line and slashdash comments are allowed in the node
                    case '/':
                    {
                        if (m_cursor + 1 < m_end && m_cursor[1] == '/')
                        {
                            if (!m_handler->EndNode())
                                return SetError(KdlReadError::Cancelled);

                            if (!CheckSlashdashEnd())
                                return false;

                            return ParseComment();
                        }

                        if (!ParseComment())
                            return false;

                        break;
                    }
                    // semicolon is a node terminator, end the node and then consume it.
                    case ';':
                    {
                        if (!m_handler->EndNode())
                            return SetError(KdlReadError::Cancelled);

                        if (!CheckSlashdashEnd())
                            return false;

                        m_cursor += len;
                        return true;
                    }
                    // open brace indicates we're starting a child block
                    case '{':
                    {
                        if (!hasWhitespace)
                            return SetError(KdlReadError::InvalidToken, ' ');

                        ++m_nodeDepth;
                        m_cursor += len;

                        do
                        {
                            if (!PeekCodePoint(ucc, len))
                                return false;

                            if (ucc == '}')
                            {
                                m_cursor += len;
                                break;
                            }

                            if (!ParseExpression())
                                return false;
                        } while (!AtEnd());

                        if (!m_handler->EndNode())
                            return SetError(KdlReadError::Cancelled);

                        if (!CheckSlashdashEnd())
                            return false;

                        return true;
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
                        if (!hasWhitespace)
                            return SetError(KdlReadError::InvalidToken, ' ');

                        if (!ParseArgumentOrProperty())
                            return false;

                        if (!CheckSlashdashEnd())
                            return false;

                        hasWhitespace = false;
                        break;
                    // anything else must be an identifier string
                    // which could be an argument value or property name
                    default:
                        if (!hasWhitespace)
                            return SetError(KdlReadError::InvalidToken, ' ');

                        if (!IsValidKdlIdentifierStartCodePoint(ucc))
                            return SetError(KdlReadError::InvalidToken);

                        if (!ParseArgumentOrProperty())
                            return false;

                        if (!CheckSlashdashEnd())
                            return false;

                        hasWhitespace = false;
                        break;
                }
            }

            // End the node if we reach EOF
            if (!m_handler->EndNode())
                return SetError(KdlReadError::Cancelled);

            if (!CheckSlashdashEnd())
                return false;

            return true;
        }

        [[nodiscard]] bool ConsumeType(StringView& type)
        {
            if (!Consume('('))
                return false;

            if (!SkipSpaces())
                return false;

            if (!ConsumeString(type))
                return false;

            if (!SkipSpaces())
                return false;

            if (!Consume(')'))
                return false;

            return true;
        }


        template <typename T>
        [[nodiscard]] bool EmitPropOrArg(T value, StringView type, const StringView* propName)
        {
            if (propName)
            {
                if (!m_handler->Property(*propName, value, type))
                    return SetError(KdlReadError::Cancelled);
            }
            else
            {
                if (!m_handler->Argument(value, type))
                    return SetError(KdlReadError::Cancelled);
            }

            return true;
        }

        [[nodiscard]] bool ParseIntFromBuffer(bool isSigned, int32_t base, StringView type, const StringView* propName)
        {
            if (isSigned)

            if (isSigned)
            {
                m_stringBuffer.PushFront('-');

                int64_t value = 0;
                const char* end = m_stringBuffer.End();
                if (!StrToInt(value, m_stringBuffer.Begin(), &end, base))
                    return SetError(KdlReadError::InvalidNumber);

                return EmitPropOrArg(value, type, propName);
            }

            uint64_t value = 0;
            const char* end = m_stringBuffer.End();
            if (!StrToInt(value, m_stringBuffer.Begin(), &end, base))
                return SetError(KdlReadError::InvalidNumber);

            return EmitPropOrArg(value, type, propName);
        }

        [[nodiscard]] bool ParseHexNum(bool isSigned, StringView type, const StringView* propName)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            m_stringBuffer.Clear();
            while (!AtEnd() && (IsHex(*m_cursor) || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            return ParseIntFromBuffer(isSigned, 16, type, propName);
        }

        [[nodiscard]] bool ParseOctNum(bool isSigned, StringView type, const StringView* propName)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            m_stringBuffer.Clear();
            while (!AtEnd() && ((*m_cursor >= '0' && *m_cursor <= '7') || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            return ParseIntFromBuffer(isSigned, 8, type, propName);
        }

        [[nodiscard]] bool ParseBinNum(bool isSigned, StringView type, const StringView* propName)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            m_stringBuffer.Clear();
            while (!AtEnd() && (*m_cursor == '0' || *m_cursor == '1' || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            return ParseIntFromBuffer(isSigned, 2, type, propName);
        }

        [[nodiscard]] bool ParseDecNum(bool isSigned, StringView type, const StringView* propName)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            const char* dot = nullptr;
            const char* exp = nullptr;
            const char* expSign = nullptr;
            bool prevWasNumeric = false;

            m_stringBuffer.Clear();

            while (!AtEnd())
            {
                uint32_t ucc = 0;
                uint32_t len = 0;
                if (!PeekCodePoint(ucc, len))
                    return false;

                if ((ucc < '0' || ucc > '9') && !IsOneOf(ucc, ".eE-+_"))
                    break;

                switch (ucc)
                {
                    case '.':
                    {
                        if (!prevWasNumeric)
                            return SetError(KdlReadError::InvalidNumber);

                        if (dot)
                            return SetError(KdlReadError::InvalidNumber);

                        dot = m_cursor;
                        break;
                    }
                    case 'e':
                    case 'E':
                    {
                        if (!prevWasNumeric)
                            return SetError(KdlReadError::InvalidNumber);

                        if (exp)
                            return SetError(KdlReadError::InvalidNumber);

                        exp = m_cursor;
                        break;
                    }
                    case '+':
                    case '-':
                    {
                        if (!exp || expSign)
                            return SetError(KdlReadError::InvalidNumber);

                        expSign = m_cursor;
                        break;
                    }
                }

                if (ucc != '_')
                    m_stringBuffer.Append(m_cursor, len);

                prevWasNumeric = ucc >= '0' && ucc <= '9';
                m_cursor += len;
            }

            if (m_stringBuffer.IsEmpty() || (m_stringBuffer.Size() == 1 && m_stringBuffer[0] == '-'))
                return SetError(KdlReadError::InvalidNumber);

            if (dot || exp)
            {
                const char* begin = m_stringBuffer.Begin();
                if (*begin == '-')
                    ++begin;

                const char* end = m_stringBuffer.End();

                // Cannot start with a dot or exponent
                if (*begin == '.' || *begin == 'e' || *begin == 'E')
                    return SetError(KdlReadError::InvalidNumber);

                // Cannot end with a dot, exponent, or sign
                if (end[-1] == '.' || end[-1] == 'e' || end[-1] == 'E' || end[-1] == '-' || end[-1] == '+')
                    return SetError(KdlReadError::InvalidNumber);

                // exponent cannot be before the dot
                if (exp && exp < dot)
                    return SetError(KdlReadError::InvalidNumber);

                double value = 0.0;
                if (!StrToFloat(value, m_stringBuffer.Begin(), &end))
                    return SetError(KdlReadError::InvalidNumber);

                return EmitPropOrArg(value, type, propName);
            }

            return ParseIntFromBuffer(isSigned, 10, type, propName);
        }

        [[nodiscard]] bool ParseNum(StringView type, const StringView* propName = nullptr)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            bool isSigned = false;
            if (*m_cursor == '-')
            {
                if (!Consume('-'))
                    return false;
                isSigned = true;
            }
            else if (*m_cursor == '+')
            {
                if (!Consume('+'))
                    return false;
            }

            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            // 0x, 0o, 0b, 0, 0.0
            if (ucc ==  '0')
            {
                m_cursor += len;

                if (!AtEnd())
                {
                    switch (*m_cursor)
                    {
                        case 'x': return Consume('x') && ParseHexNum(isSigned, type, propName);
                        case 'o': return Consume('o') && ParseOctNum(isSigned, type, propName);
                        case 'b': return Consume('b') && ParseBinNum(isSigned, type, propName);
                    }
                }

                // Back up the cursor so the leading zero is restored, and fall through to
                // decimal number parsing in the `default` case.
                m_cursor -= len;
            }

            return ParseDecNum(isSigned, type, propName);
        }

        [[nodiscard]] bool ParseArgumentOrProperty(const StringView* propName = nullptr)
        {
            if (!SkipSpaces())
                return false;

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            // Consume the type annotation if present
            StringView type;
            if (ucc == '(')
            {
                if (!ConsumeType(type))
                    return false;

                if (!SkipSpaces())
                    return false;

                if (!PeekCodePoint(ucc, len))
                    return false;
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
                            return false;

                        return EmitPropOrArg(value, type, propName);
                    }

                    // otherwise, this is a keyword value
                    m_cursor += len;
                    switch (*m_cursor)
                    {
                        case 't': return Consume("true") ? EmitPropOrArg(true, type, propName) : false;
                        case 'f': return Consume("false") ? EmitPropOrArg(false, type, propName) : false;
                        case 'i': return Consume("inf") ? EmitPropOrArg(Limits<double>::Infinity, type, propName) : false;
                        case '-': return Consume("-inf") ? EmitPropOrArg(-Limits<double>::Infinity, type, propName) : false;
                        case 'n':
                        {
                            if (m_cursor + 1 < m_end && m_cursor[1] == 'a')
                                return Consume("nan") ? EmitPropOrArg(Limits<double>::NaN, type, propName) : false;

                            return Consume("null") ? EmitPropOrArg(nullptr, type, propName) : false;
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
                    ParseNum(type, propName);
                    break;
                // could be a number or the start of an identifier string
                case '-':
                case '+':
                {
                    if (m_cursor + len < m_end && m_cursor[len] != '\0' && IsNumeric(m_cursor[len]))
                    {
                        return ParseNum(type, propName);
                    }

                    // fall through to string parsing
                    [[fallthrough]];
                }
                // anything else must be a string
                default:
                {
                    StringView value;
                    if (!ConsumeString(value))
                        return false;

                    if (!SkipSpaces())
                        return false;

                    if (!PeekCodePoint(ucc, len))
                        return false;

                    if (IsKdlEqualsSign(ucc))
                    {
                        if (propName)
                            return SetError(KdlReadError::InvalidToken);

                        m_cursor += len;
                        return ParseArgumentOrProperty(&value);
                    }

                    return EmitPropOrArg(value, type, propName);
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
        uint32_t m_nodeDepth{ 0 };
        uint32_t m_slashDashDepth{ 0 };

        String m_stringBuffer;
    };

    // --------------------------------------------------------------------------------------------
    KdlReader::KdlReader(Allocator& allocator = Allocator::GetDefault()) noexcept
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
            case KdlReadError::InvalidBom: return "InvalidBom";
            case KdlReadError::InvalidEscapeSequence: return "InvalidEscapeSequence";
            case KdlReadError::InvalidControlChar: return "InvalidControlChar";
            case KdlReadError::InvalidIdentifier: return "InvalidKey";
            case KdlReadError::InvalidNumber: return "InvalidNumber";
            case KdlReadError::InvalidToken: return "InvalidToken";
            case KdlReadError::InvalidDocument: return "InvalidDocument";
        }
    }
}
