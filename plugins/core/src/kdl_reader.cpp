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
                        return false;

                    // Single line comments and slashdash comments are terminators for nodes, they don't count just "spaces"
                    if (ucc == '/')
                        return true;

                    // Some contexts don't allow slashdash comments
                    if (!allowSlashdash && ucc == '-')
                        return true;

                    if (!ParseComment())
                        return false;
                }
                // if it is whitespace consume it and continue
                else if (IsKdlWhitespace(ucc))
                {
                    m_cursor += len;
                }
                // escaped newlines are considered normal spaces
                else if (IsKdlNewline(ucc) && m_inWhitespaceEscape)
                {
                    if (!ConsumeNewLine())
                        return false;
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

        [[nodiscard]] bool Consume(char ch)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            if (ucc != static_cast<uint32_t>(ch))
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
                if (ucc == static_cast<uint32_t>(*chars++))
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
                if (!IsEndOfString(begin, m_end, rawDelimCount))
                    continue;

                // Store the dedent prefix
                outDedentPrefix = { lineStart, begin - 1 }; // -1 for the quote
                break;
            }

            // Validate that the final line of the string is only whitespace
            for (const uint32_t ucc : Utf8Splitter(outDedentPrefix))
            {
                if (!IsKdlWhitespace(ucc))
                    return SetError(KdlReadError::InvalidToken, ' ');
            }

            return true;
        }

        [[nodiscard]] bool ConsumeType(bool& hasType)
        {
            if (!Consume('('))
                return false;

            if (!SkipSpaces())
                return false;

            StringView type;
            if (!ConsumeString(type))
                return false;

            if (!SkipSpaces())
                return false;

            if (!Consume(')'))
                return false;

            if (!SkipSpaces())
                return false;

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

            // Iterating bytes because the depent prefix must be byte-wise identical to be valid.
            for (const char ch : dedentPrefix)
            {
                if (*m_cursor != ch)
                {
                    return SetError(KdlReadError::InvalidToken, ch);
                }

                ++m_cursor;
            }

            return true;
        }

        [[nodiscard]] bool ConsumeUnicodeEscapeSequence(String& dst)
        {
            if (!Consume('u'))
                return false;

            if (!Consume('{'))
                return SetError(KdlReadError::InvalidEscapeSequence);

            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            // Max of 6 hex characters
            bool foundNum = false;
            uint32_t ucc = 0;
            for (uint32_t i = 0; i < 6; ++i)
            {
                if (AtEnd())
                    break;

                if (*m_cursor == '}')
                    break;

                if (!IsHex(*m_cursor))
                    return SetError(KdlReadError::InvalidEscapeSequence);

                ucc = (ucc << 4) + HexToNibble(*m_cursor);
                foundNum = true;
                ++m_cursor;
            }

            if (!Consume('}'))
                return SetError(KdlReadError::InvalidEscapeSequence);

            if (!foundNum)
                return SetError(KdlReadError::InvalidEscapeSequence);

            if (!IsKdlUnicodeScalarValue(ucc))
                return SetError(KdlReadError::InvalidEscapeSequence);

            const uint32_t len = UTF8Encode(dst, ucc);
            return len ? true : SetError(KdlReadError::InvalidEscapeSequence);
        }

        [[nodiscard]] bool ConsumeQuotedString(String& dst, uint32_t rawDelimCount)
        {
            dst.Clear();

            bool firstChar = true;
            bool isMultiline = false;
            bool inEscapeSeq = false;
            bool inWhitespaceEscape = false;
            StringView dedentPrefix;
            while (!AtEnd())
            {
                uint32_t ucc = 0;
                uint32_t len = 0;
                if (!PeekCodePoint(ucc, len))
                    return false;

                // Multi-line string handling
                if (firstChar)
                {
                    isMultiline = IsKdlNewline(ucc);
                    firstChar = false;

                    if (isMultiline)
                    {
                        if (!ConsumeNewLine())
                            return false;

                        // Multi-line strings dedent by the number of spaces on the last line
                        // of the string. This means we need to scan to the end of the string
                        // and store the whitespace prefix of the final line.
                        if (!GetDedentPrefix(dedentPrefix, rawDelimCount))
                            return false;

                        if (!ConsumeDedentPrefix(dedentPrefix))
                            return false;

                        continue;
                    }
                }

                // Escape sequence handling
                if (inEscapeSeq)
                {
                    // skip whitespace after escape sequence
                    if (IsKdlWhitespace(ucc))
                    {
                        inWhitespaceEscape = true;
                        m_cursor += len;
                        continue;
                    }

                    // skip newlines after escape sequence
                    if (IsKdlNewline(ucc))
                    {
                        inWhitespaceEscape = true;

                        if (!ConsumeNewLine())
                            return false;

                        if (!ConsumeDedentPrefix(dedentPrefix))
                            return false;

                        continue;
                    }

                    if (inWhitespaceEscape)
                    {
                        inEscapeSeq = false;
                        inWhitespaceEscape = false;
                        continue;
                    }

                    if (ucc == 'u')
                    {
                        if (!ConsumeUnicodeEscapeSequence(dst))
                            return false;

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
                        default:
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
                    if (!isMultiline)
                        return SetError(KdlReadError::InvalidControlChar);

                    if (!ConsumeNewLine())
                        return false;

                    // Consume the required dedent prefix since we consumed a newline
                    if (!ConsumeDedentPrefix(dedentPrefix))
                        return false;

                    // newlines are normalized to `\n`
                    dst.PushBack('\n');
                    continue;
                }

                // New escape sequence start handling
                if (ucc == '\\')
                {
                    if (rawDelimCount > 0)
                        dst.PushBack('\\');
                    else
                        inEscapeSeq = true;

                    m_cursor += len;
                    continue;
                }

                // End of string handling
                if (ucc == '"')
                {
                    if (IsEndOfString(m_cursor + len, m_end, rawDelimCount))
                    {
                        m_cursor += len;
                        m_cursor += rawDelimCount;
                        if (!dst.IsEmpty() && dst.Back() == '\n')
                            dst.PopBack();
                        return true;
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
                    return false;

                if (first && !IsValidKdlIdentifierStartCodePoint(ucc))
                    return SetError(KdlReadError::InvalidIdentifier);

                first = false;

                if (!IsValidKdlIdentifierCodePoint(ucc))
                    break;

                m_cursor += len;
            }

            value = { begin, m_cursor };

            // No identifier string can start with a number
            if (IsNumeric(value[0]))
                return SetError(KdlReadError::InvalidIdentifier);

            if (value[0] == '-' || value[0] == '+')
            {
                // Identifiers can start with "-."/"+." as long as the next char is not a number
                if (value.Size() > 2 && value[1] == '.' && IsNumeric(value[2]))
                    return SetError(KdlReadError::InvalidIdentifier);

                // Identifiers can start with "-"/"+" as long as the next char is not a number
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
                case '}':
                {
                    if (!EmitEndNode())
                        return false;

                    m_cursor += len;
                    return true;
                }
                case '(':
                case '"':
                case '#':
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
            if (m_slashDashDepthStack.IsEmpty())
                return true;

            if (m_slashDashDepthStack.Back() == m_nodeDepth + 1)
            {
                m_slashDashDepthStack.PopBack();
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
                    m_slashDashDepthStack.PushBack(m_nodeDepth + 1);

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
                        if (!PeekCodePoint(ucc, len))
                            return false;

                        if (IsKdlNewline(ucc))
                            break;

                        m_cursor += len;
                    }

                    if (!m_handler->Comment({ begin, m_cursor }))
                        return SetError(KdlReadError::Cancelled);

                    if (!AtEnd())
                    {
                        if (!ConsumeNewLine())
                            return false;
                    }

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

            bool hasType = false;
            if (ucc == '(')
            {
                if (!ConsumeType(hasType))
                    return false;
            }

            StringView name;
            if (!ConsumeString(name))
                return false;

            if (!EmitStartNode(name, hasType))
                return false;

            if (AtEnd())
            {
                if (!EmitEndNode())
                    return false;

                return true;
            }

            // Next must be: whitespace, newline, semicolon, single-line comment
            if (!PeekCodePoint(ucc, len))
                return false;

            if (!IsKdlWhitespace(ucc) && !IsKdlNewline(ucc) && ucc != ';' && ucc != '/' && ucc != '}')
                return SetError(KdlReadError::InvalidToken);

            bool hasWhitespace = false;
            while (!AtEnd())
            {
                if (!PeekCodePoint(ucc, len))
                    return false;

                // Newline is a node terminator, end the node and then consume it.
                if (IsKdlNewline(ucc))
                {
                    if (!EmitEndNode())
                        return false;

                    return ConsumeNewLine();
                }

                // Whitespace is required between the node name and the first argument or property
                if (IsKdlWhitespace(ucc))
                {
                    hasWhitespace = true;
                    if (!SkipSpaces())
                        return false;

                    continue;
                }

                // consume an argument, property, or start of children
                switch (ucc)
                {
                    // single-line comment is a node terminator, end the node and then consume it.
                    // multi-line and slashdash comments are allowed in the node
                    case '/':
                    {
                        if (m_cursor + 1 < m_end && m_cursor[1] == '/' && !m_inWhitespaceEscape)
                        {
                            if (!EmitEndNode())
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
                        if (!EmitEndNode())
                            return false;

                        m_cursor += len;
                        return true;
                    }
                    // end brace is a node terminator, end the node but don't consume it.
                    // Let ParseExpression consume it to end the parent node.
                    case '}':
                    {
                         if (!EmitEndNode())
                            return false;

                        return true;
                    }
                    // open brace indicates we're starting a child block
                    case '{':
                    {
                        if (!hasWhitespace)
                            return SetError(KdlReadError::InvalidToken, ' ');

                        m_cursor += len;

                        const uint32_t depth = m_nodeDepth;
                        while (m_nodeDepth >= depth)
                        {
                            if (AtEnd())
                                return SetError(KdlReadError::UnexpectedEof);

                            if (!ParseExpression())
                                return false;
                        }

                        // Note: node end will be emitted by ParseExpression.
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
                    {
                        if (!hasWhitespace)
                            return SetError(KdlReadError::InvalidToken, ' ');

                        if (!ParseArgumentOrProperty())
                            return false;

                        hasWhitespace = false;
                        break;
                    }
                    // anything else must be an identifier string
                    // which could be an argument value or property name
                    default:
                    {
                        if (!hasWhitespace)
                            return SetError(KdlReadError::InvalidToken, ' ');

                        if (!IsValidKdlIdentifierStartCodePoint(ucc))
                            return SetError(KdlReadError::InvalidToken);

                        if (!ParseArgumentOrProperty())
                            return false;

                        hasWhitespace = false;
                        break;
                    }
                }
            }

            // End the node if we reach EOF
            if (!EmitEndNode())
                return false;

            return true;
        }

        [[nodiscard]] bool EmitStartNode(StringView name, bool hasType)
        {
            StringView typeViewStorage = hasType ? m_typeBuffer : "";
            const StringView* typeView = hasType ? &typeViewStorage : nullptr;

            if (!m_handler->StartNode(name, typeView))
                return SetError(KdlReadError::Cancelled);

            ++m_nodeDepth;
            return true;
        }

        [[nodiscard]] bool EmitEndNode()
        {
            if (m_nodeDepth == 0)
                return SetError(KdlReadError::InvalidDocument);

            if (!m_handler->EndNode())
                return SetError(KdlReadError::Cancelled);

            --m_nodeDepth;
            return CheckSlashdashEnd();
        }

        template <typename T>
        [[nodiscard]] bool EmitPropOrArg(T value, bool hasType, const StringView* propName)
        {
            StringView typeViewStorage = hasType ? m_typeBuffer : "";
            const StringView* typeView = hasType ? &typeViewStorage : nullptr;

            if (propName)
            {
                if (!m_handler->Property(*propName, value, typeView))
                    return SetError(KdlReadError::Cancelled);
            }
            else
            {
                if (!m_handler->Argument(value, typeView))
                    return SetError(KdlReadError::Cancelled);
            }

            return CheckSlashdashEnd();
        }

        [[nodiscard]] bool ParseIntFromBuffer(uint32_t base, bool hasType, const StringView* propName)
        {
            if (m_stringBuffer.IsEmpty())
                return SetError(KdlReadError::InvalidNumber);

            if (m_stringBuffer[0] == '-')
            {
                int64_t value = 0;
                const char* end = m_stringBuffer.End();
                if (!StrToInt(value, m_stringBuffer.Begin(), &end, base))
                    return SetError(KdlReadError::InvalidNumber);

                return EmitPropOrArg(value, hasType, propName);
            }

            uint64_t value = 0;
            const char* end = m_stringBuffer.End();
            if (!StrToInt(value, m_stringBuffer.Begin(), &end, base))
                return SetError(KdlReadError::InvalidNumber);

            return EmitPropOrArg(value, hasType, propName);
        }

        [[nodiscard]] bool ParseHexNum(bool hasType, const StringView* propName)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            if (*m_cursor == '_')
                return SetError(KdlReadError::InvalidNumber);

            while (!AtEnd() && (IsHex(*m_cursor) || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            return ParseIntFromBuffer(16, hasType, propName);
        }

        [[nodiscard]] bool ParseOctNum(bool hasType, const StringView* propName)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            if (*m_cursor == '_')
                return SetError(KdlReadError::InvalidNumber);

            while (!AtEnd() && ((*m_cursor >= '0' && *m_cursor <= '7') || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            return ParseIntFromBuffer(8, hasType, propName);
        }

        [[nodiscard]] bool ParseBinNum(bool hasType, const StringView* propName)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            if (*m_cursor == '_')
                return SetError(KdlReadError::InvalidNumber);

            while (!AtEnd() && (*m_cursor == '0' || *m_cursor == '1' || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            return ParseIntFromBuffer(2, hasType, propName);
        }

        [[nodiscard]] bool ParseDecNum(bool hasType, const StringView* propName)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            const char* dot = nullptr;
            const char* exp = nullptr;
            const char* expSign = nullptr;
            bool prevWasNumeric = false;

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
                    case '_':
                    {
                        if (dot == (m_cursor - len))
                            return SetError(KdlReadError::InvalidNumber);

                        break;
                    }
                }

                if (ucc != '_')
                    m_stringBuffer.Append(m_cursor, len);

                prevWasNumeric = ucc >= '0' && ucc <= '9';
                m_cursor += len;
            }

            if (dot || exp)
            {
                if (m_stringBuffer.IsEmpty())
                    return SetError(KdlReadError::InvalidNumber);

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

                return EmitPropOrArg(value, hasType, propName);
            }

            return ParseIntFromBuffer(10, hasType, propName);
        }

        [[nodiscard]] bool ParseNum(bool hasType, const StringView* propName = nullptr)
        {
            if (AtEnd())
                return SetError(KdlReadError::UnexpectedEof);

            m_stringBuffer.Clear();

            if (*m_cursor == '-')
            {
                if (!Consume('-'))
                    return false;

                m_stringBuffer.PushBack('-');
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
            if (!SkipSpaces())
                return false;

            uint32_t ucc = 0;
            uint32_t len = 0;
            if (!PeekCodePoint(ucc, len))
                return false;

            // Consume the type annotation if present
            bool hasType = false;
            if (ucc == '(')
            {
                if (!ConsumeType(hasType))
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
                                return Consume("nan") ? EmitPropOrArg(Limits<double>::NaN, hasType, propName) : false;

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
                    return ParseNum(hasType, propName);
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
                        return false;

                    if (AtEnd())
                        return EmitPropOrArg(value, hasType, propName);

                    const char* beforeSpaces = m_cursor;
                    if (!SkipSpaces(false))
                        return false;

                    if (AtEnd())
                        return EmitPropOrArg(value, hasType, propName);

                    if (!PeekCodePoint(ucc, len))
                        return false;

                    if (IsKdlEqualsSign(ucc))
                    {
                        if (propName)
                            return SetError(KdlReadError::InvalidToken);

                        if (hasType)
                            return SetError(KdlReadError::InvalidToken);

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
            case KdlReadError::InvalidUtf8: return "InvalidUtf8";
            case KdlReadError::InvalidEscapeSequence: return "InvalidEscapeSequence";
            case KdlReadError::InvalidControlChar: return "InvalidControlChar";
            case KdlReadError::InvalidIdentifier: return "InvalidIdentifier";
            case KdlReadError::InvalidNumber: return "InvalidNumber";
            case KdlReadError::InvalidToken: return "InvalidToken";
            case KdlReadError::InvalidDocument: return "InvalidDocument";
        }

        return "<unknown>";
    }
}
