// Copyright Chad Engler

#include "he/core/toml_reader.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/limits.h"
#include "he/core/string_fmt.h"
#include "he/core/utf8.h"
#include "he/core/vector.h"

#include <time.h>

namespace he
{
    // --------------------------------------------------------------------------------------------
    class TomlParser
    {
    public:
        TomlReadResult Parse(StringView src, TomlReader::Handler& handler)
        {
            m_result = {};
            m_handler = &handler;
            m_cursor = src.Begin();
            m_end = src.End();
            m_lineStart = m_cursor;
            m_line = 1;
            m_pathBuffer.Clear();

            if (!SkipBOM())
            {
                SetError(TomlReadError::InvalidBom);
                return m_result;
            }

            m_handler->StartDocument();

            if (!ParseExpression())
                return m_result;

            while (!AtEnd())
            {
                if (!ParseNewLine())
                    return m_result;

                if (!ParseExpression())
                    return m_result;
            }

            m_handler->EndDocument();

            return m_result;
        }

    private:
        bool SetError(TomlReadError error, char expected = '\0')
        {
            const uint32_t column = static_cast<uint32_t>(m_cursor - m_lineStart);
            m_result = { error, m_line, column, expected };
            return false;
        }

        [[nodiscard]] bool AtEnd() const { return m_cursor >= m_end || *m_cursor == '\0'; }

        [[nodiscard]] bool SkipBOM()
        {
            if (static_cast<uint8_t>(*m_cursor) != 0xef)
                return true;

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbb)
                return SetError(TomlReadError::InvalidBom);

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbf)
                return SetError(TomlReadError::InvalidBom);

            ++m_cursor;
            return true;
        }

        void SkipSpaces()
        {
            while (!AtEnd() && (*m_cursor == ' ' || *m_cursor == '\t'))
                ++m_cursor;
        }

        void SkipSpacesAndNewlines()
        {
            while (!AtEnd())
            {
                SkipSpaces();

                if (AtEnd())
                    break;

                if (*m_cursor == '#')
                {
                    ParseComment();
                    ParseNewLine();
                }
                else if (*m_cursor == '\r' || *m_cursor == '\n')
                {
                    ParseNewLine();
                }
                else
                {
                    break;
                }
            }
        }

        [[nodiscard]] bool Consume(char ch)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            if (*m_cursor != ch)
                return SetError(TomlReadError::InvalidToken, ch);

            ++m_cursor;
            return true;
        }

        [[nodiscard]] bool ConsumeOneOf(const char* chars)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            while (*chars)
            {
                if (*m_cursor == *chars)
                {
                    ++m_cursor;
                    return true;
                }
                ++chars;
            }

            return SetError(TomlReadError::InvalidToken);
        }

        [[nodiscard]] static bool IsOneOf(char ch, const char* chars)
        {
            while (*chars)
            {
                if (ch == *chars++)
                    return true;
            }

            return false;
        }

    private:
        bool ParseNewLine()
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            if (*m_cursor == '\r')
            {
                if (!Consume('\r'))
                    return false;

                if (!Consume('\n'))
                    return SetError(TomlReadError::InvalidNewline);
            }
            else if (!Consume('\n'))
            {
                return false;
            }

            ++m_line;
            m_lineStart = m_cursor;
            return true;
        }

        bool ParseComment()
        {
            if (!Consume('#'))
                return false;

            const char* begin = m_cursor;
            while (!AtEnd() && *m_cursor != '\r' && *m_cursor != '\n')
            {
                const char ch = *m_cursor;
                if (ch == '\x00' || (ch >= '\x0A' && ch <= '\x0D'))
                    return SetError(TomlReadError::InvalidToken);

                ++m_cursor;
            }

            m_handler->Comment(StringView(begin, m_cursor));
            return true;
        }

        bool ParseExpression()
        {
            SkipSpaces();

            if (AtEnd())
                return true;

            switch (*m_cursor)
            {
                case '\r':
                case '\n':
                    return true;
                case '#':
                    return ParseComment();
                case '[':
                {
                    if (!ParseTableHeader())
                        return false;

                    SkipSpaces();

                    if (!AtEnd() && *m_cursor == '#')
                        return ParseComment();

                    return true;
                }
                default:
                    if (!ParseKeyValuePair())
                        return false;

                    SkipSpaces();

                    if (!AtEnd() && *m_cursor == '#')
                        return ParseComment();

                    return true;
            }
        }

        bool ParseTableHeader()
        {
            if (!Consume('['))
                return false;

            const bool isArray = *m_cursor == '[';
            if (isArray)
            {
                if (!Consume('['))
                    return false;
            }

            m_pathBuffer.Clear();
            if (!ParseKey())
                return false;

            SkipSpaces();

            if (!Consume(']'))
                return false;

            if (isArray)
            {
                if (!Consume(']'))
                    return false;
            }

            m_handler->Table(m_pathBuffer, isArray);
            return true;
        }

        bool ParseKeyValuePair()
        {
            if (!ParseKey())
                return false;

            m_handler->Key(m_pathBuffer);

            SkipSpaces();

            if (!Consume('='))
                return false;

            SkipSpaces();

            if (!ParseValue())
                return false;

            m_pathBuffer.PopBack();
            return true;
        }

        bool ParseKey()
        {
            while (true)
            {
                SkipSpaces();

                if (AtEnd())
                    return SetError(TomlReadError::UnexpectedEof);

                String& dst = m_pathBuffer.EmplaceBack();

                if (*m_cursor == '"')
                {
                    if (!ParseBasicString(dst, false))
                        return false;
                }
                else if (*m_cursor == '\'')
                {
                    if (!ParseLiteralString(dst, false))
                        return false;
                }
                else
                {
                    const char* begin = m_cursor;

                    while (!AtEnd() && !IsWhitespace(*m_cursor) && !IsOneOf(*m_cursor, "].="))
                    {
                        const uint32_t ucc = FromUTF8(m_cursor);

                        if (ucc == static_cast<uint32_t>(-1))
                            return SetError(TomlReadError::InvalidKey);

                        const bool isValid =
                            // a-z A-Z 0-9 - _
                            (ucc >= 'a' && ucc <= 'z')
                            || (ucc >= 'A' && ucc <= 'Z')
                            || (ucc >= '0' && ucc <= '9')
                            || (ucc == '_' || ucc == '-')
                            // non-symbol chars in Latin block
                            || (ucc >= 0xC0 && ucc <= 0xD6)
                            || (ucc >= 0xD8 && ucc <= 0xF6)
                            || (ucc >= 0xF8 && ucc <= 0xFF)
                            // this excludes GREEK QUESTION MARK, which is basically a semi-colon
                            || (ucc >= 0x0100 && ucc <= 0x02FF)
                            || (ucc >= 0x0300 && ucc <= 0x037D)
                            || (ucc >= 0x037F && ucc <= 0x1FFF)
                            // include combining chars used in some languages
                            || (ucc >= 0x200C && ucc <= 0x200D)
                            || (ucc >= 0x203F && ucc <= 0x2040)
                            // this excludes arrows, blocks and the like
                            || (ucc >= 0x2070 && ucc <= 0x218F)
                            || (ucc >= 0x2C00 && ucc <= 0x2FEF)
                            // skip 2FF0-3000 ideographic up/down markers and spaces
                            || (ucc >= 0x3001 && ucc <= 0xD7FF)
                            // skip D800-D999 surrogate block, E000-F8FF Private Use area,
                            // FDD0-FDEF intended for process-internal use (unicode)
                            || (ucc >= 0xF900 && ucc <= 0xFDCF)
                            || (ucc >= 0xFDF0 && ucc <= 0xFFFD)
                            // all chars outside BMP range, excluding Private Use planes
                            || (ucc >= 0x10000 && ucc <= 0xEFFFF);

                        if (!isValid)
                            return SetError(TomlReadError::InvalidKey);
                    }

                    if (begin == m_cursor)
                        return SetError(TomlReadError::InvalidKey);

                    if (AtEnd())
                        return SetError(TomlReadError::UnexpectedEof);

                    dst.Assign(begin, m_cursor);
                }

                SkipSpaces();

                if (!AtEnd() && IsWhitespace(*m_cursor))
                    return SetError(TomlReadError::InvalidKey);

                if (AtEnd() || *m_cursor != '.')
                    return true;

                if (!Consume('.'))
                    return false;
            }
        }

        bool ParseValue()
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            switch (*m_cursor)
            {
                case '[':
                    return ParseArray();
                case '{':
                    return ParseInlineTable();
                case '"':
                    if (!ParseBasicString(m_stringBuffer, true))
                        return false;
                    m_handler->String(m_stringBuffer);
                    return true;
                case '\'':
                    if (!ParseLiteralString(m_stringBuffer, true))
                        return false;
                    m_handler->String(m_stringBuffer);
                    return true;
                case 't':
                    return ParseTrue();
                case 'f':
                    return ParseFalse();
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
                    return ParseDateTimeOrNum();
                case '+':
                case '-':
                case 'i':
                case 'n':
                    return ParseNum();
                default:
                    return SetError(TomlReadError::InvalidToken);
            }
        }

        template <bool IsArray>
        bool ParseInlineStructure()
        {
            constexpr char OpenBrace = IsArray ? '[' : '{';
            constexpr char CloseBrace = IsArray ? ']' : '}';

            if (!Consume(OpenBrace))
                return false;

            if constexpr (IsArray)
                m_handler->StartArray();
            else
                m_handler->StartInlineTable();

            SkipSpacesAndNewlines();

            uint32_t count = 0;
            while (!AtEnd())
            {
                if (*m_cursor == CloseBrace)
                {
                    if (!Consume(CloseBrace))
                        return false;

                    if constexpr (IsArray)
                        m_handler->EndArray(count);
                    else
                        m_handler->EndInlineTable(count);

                    return true;
                }

                if constexpr (IsArray)
                {
                    if (!ParseValue())
                        return false;
                }
                else
                {
                    if (!ParseKeyValuePair())
                        return false;
                }

                ++count;
                SkipSpacesAndNewlines();

                if (*m_cursor != ',' && *m_cursor != CloseBrace)
                    return SetError(TomlReadError::InvalidToken);

                if (*m_cursor == ',')
                {
                    if (!Consume(','))
                        return false;

                    SkipSpacesAndNewlines();
                }
            }

            return SetError(TomlReadError::UnexpectedEof);
        }

        bool ParseArray()
        {
            return ParseInlineStructure<true>();
        }

        bool ParseInlineTable()
        {
            return ParseInlineStructure<false>();
        }

        bool ParseBasicString(String& dst, bool allowMultiline)
        {
            return ParseGenericString(dst, '"', true, allowMultiline);
        }

        bool ParseLiteralString(String& dst, bool allowMultiline)
        {
            return ParseGenericString(dst, '\'', false, allowMultiline);
        }

        bool ParseGenericString(String& dst, char quote, bool decodeEscapeCharacters, bool allowMultiline)
        {
            dst.Clear();

            if (!Consume(quote))
                return false;

            // Check opening quotes for multiline string. We've already consumed one quote to get
            // here, so we check for a second and third quote.
            bool isMultiline = false;
            if (*m_cursor == quote)
            {
                ++m_cursor;
                if (allowMultiline && !AtEnd() && *m_cursor == quote)
                {
                    ++m_cursor;
                    isMultiline = true;
                }
                else
                {
                    // empty string
                    return true;
                }
            }

            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            // Skip a single newline if it immediately follows the opening of a multiline string
            if (isMultiline && (*m_cursor == '\r' || *m_cursor == '\n'))
            {
                if (!ParseNewLine())
                    return false;
            }

            // Decode the string into our buffer
            while (true)
            {
                if (AtEnd())
                    return SetError(TomlReadError::UnexpectedEof);

                // Unescaped newlines are allowed in multiline strings.
                // Here we also normalize `\r\n` sequences to `\n`.
                if (isMultiline && (*m_cursor == '\r' || *m_cursor == '\n'))
                {
                    if (!ParseNewLine())
                        return false;

                    dst.PushBack('\n');
                    continue;
                }

                char ch = *m_cursor++;

                // Tab characters are explicitly allowed.
                if (ch == '\t')
                {
                    dst.PushBack('\t');
                    continue;
                }

                // Unescaped control characters are not allowed in strings.
                if ((ch >= '\x00' && ch <= '\x1F') || ch == '\x7F')
                    return SetError(TomlReadError::InvalidControlChar);

                // Check for string ending quotes. For multiline strings you can actually put
                // literal quotes in the string. Only a series of three quotes (""") ends it.
                if (ch == quote)
                {
                    if (!isMultiline)
                        return true;

                    if (!AtEnd() && *m_cursor == quote)
                    {
                        ++m_cursor;

                        if (!AtEnd() && *m_cursor == quote)
                        {
                            ++m_cursor;
                            return true;
                        }
                        dst.PushBack(quote);
                    }
                    dst.PushBack(quote);
                    continue;
                }

                // Handle escaped characters
                if (decodeEscapeCharacters && ch == '\\')
                {
                    if (AtEnd())
                        return SetError(TomlReadError::UnexpectedEof);

                    switch (*m_cursor)
                    {
                        case 'b': dst.PushBack('\b'); ++m_cursor; break;
                        case 't': dst.PushBack('\t'); ++m_cursor; break;
                        case 'n': dst.PushBack('\n'); ++m_cursor; break;
                        case 'f': dst.PushBack('\f'); ++m_cursor; break;
                        case 'r': dst.PushBack('\r'); ++m_cursor; break;
                        case 'e': dst.PushBack('\x1B'); ++m_cursor; break;
                        case '"': dst.PushBack('"'); ++m_cursor; break;
                        case '\\': dst.PushBack('\\'); ++m_cursor; break;
                        case '\r':
                        case '\n':
                        {
                            // Escaped newlines indicate that we ignore all whitespace until the
                            // next non-whitespace character
                            while (m_cursor < m_end && IsWhitespace(*m_cursor))
                            {
                                if (*m_cursor == '\r' || *m_cursor == '\n')
                                {
                                    if (!ParseNewLine())
                                        return false;
                                }
                                else
                                {
                                    ++m_cursor;
                                }
                            }
                            break;
                        }
                        case 'x':
                        {
                            if (!Consume('x'))
                                return false;

                            if (!ParseUnicodeCodePoint(dst, 2))
                                return false;

                            break;
                        }
                        case 'u':
                        {
                            if (!Consume('u'))
                                return false;

                            if (!ParseUnicodeCodePoint(dst, 4))
                                return false;

                            break;
                        }
                        case 'U':
                        {
                            if (!Consume('U'))
                                return false;

                            if (!ParseUnicodeCodePoint(dst, 8))
                                return false;

                            break;
                        }
                        default:
                            return SetError(TomlReadError::InvalidEscapeSequence);
                    }

                    continue;
                }

                // Totally normal character that just gets pushed into our decode buffer
                dst.PushBack(ch);
            }
        }

        bool ParseUnicodeCodePoint(String& dst, uint32_t len)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            // Validate each of the characters and read their values into codepoint
            uint32_t codepoint = 0;
            for (uint32_t i = 0; i < len; ++i)
            {
                if (!IsHex(m_cursor[i]))
                    return SetError(TomlReadError::InvalidUnicode);

                codepoint = (codepoint << 4) + HexToNibble(m_cursor[i]);
            }

            // Surrogate pairs are not allowed
            if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
                return SetError(TomlReadError::InvalidUnicode);

            // Valid Unicode range for TOML is <= 0x10FFFF
            if (codepoint > 0x10FFFF)
                return SetError(TomlReadError::InvalidUnicode);

            m_cursor += len;

            if (codepoint <= 0x7F)
            {
                dst.PushBack(static_cast<uint8_t>(codepoint));
            }
            else if (codepoint <= 0x7FF)
            {
                dst.PushBack(static_cast<uint8_t>(0xC0 | (codepoint >> 6)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            else if (codepoint <= 0xFFFF)
            {
                dst.PushBack(static_cast<uint8_t>(0xE0 | (codepoint >> 12)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            else
            {
                dst.PushBack(static_cast<uint8_t>(0xF0 | (codepoint >> 18)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }

            return true;
        }

        bool ParseTrue()
        {
            if (!Consume('t') || !Consume('r') || !Consume('u') || !Consume('e'))
                return false;

            m_handler->Bool(true);
            return true;
        }

        bool ParseFalse()
        {
            if (!Consume('f') || !Consume('a') || !Consume('l') || !Consume('s') || !Consume('e'))
                return false;

            m_handler->Bool(false);
            return true;
        }

        bool ParseDateTimeOrNum()
        {
            const uint32_t slack = static_cast<uint32_t>(m_end - m_cursor);

            if (slack >= 5)
            {
                // 4 digits and a dash indicates an "Offset Date-Time", "Local Date-Time",
                // or "Local Date".
                if (IsNumeric(m_cursor[0])
                    && IsNumeric(m_cursor[1])
                    && IsNumeric(m_cursor[2])
                    && IsNumeric(m_cursor[3])
                    && m_cursor[4] == '-')
                {
                    return ParseDateTime();
                }

                // 2 digits : 2 digits indicates a "Local Time"
                if (IsNumeric(m_cursor[0])
                    && IsNumeric(m_cursor[1])
                    && m_cursor[2] == ':'
                    && IsNumeric(m_cursor[3])
                    && IsNumeric(m_cursor[4]))
                {
                    return ParseTime();
                }
            }

            return ParseNum();
        }

        bool ParseTime()
        {
            constexpr const char Nums[] = "0123456789";

            uint32_t hours = 0;
            uint32_t minutes = 0;
            double seconds = 0.0;

            // 2 digit hour
            const char* begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                 return false;

            hours = String::ToInteger<uint32_t>(begin, m_cursor);

            if (!Consume(':'))
                 return false;

            // 2 digit minute
            begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                 return false;

            minutes = String::ToInteger<uint32_t>(begin, m_cursor);

            // 2 digit seconds are optional
            if (!AtEnd() && *m_cursor == ':')
            {
                if (!Consume(':'))
                    return false;

                begin = m_cursor;
                if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                    return false;

                // Optional fractional seconds
                if (!AtEnd() && *m_cursor == '.')
                {
                    ++m_cursor;

                    if (!ConsumeOneOf(Nums))
                        return false;

                    while (!AtEnd() && IsNumeric(*m_cursor))
                        ++m_cursor;
                }

                seconds = String::ToFloat<double>(begin, m_cursor);
            }

            if (hours >= 23 || minutes >= 60 || seconds >= 60.0)
                return SetError(TomlReadError::InvalidDateTime);

            const Duration value = FromPeriod<Hours>(hours) + FromPeriod<Minutes>(minutes) + FromPeriod<Seconds>(seconds);
            m_handler->Time(value);
            return true;
        }

        bool ParseDateTime()
        {
            constexpr const char Nums[] = "0123456789";
            const Duration localTzOffset = GetLocalTimezoneOffset();

            uint32_t year = 0;
            uint32_t month = 0;
            uint32_t day = 0;
            uint32_t hours = 0;
            uint32_t minutes = 0;
            double seconds = 0.0;
            Duration tzOffset = Duration_Zero;

            // 4 digit year
            const char* begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums) || !ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                return false;

            year = String::ToInteger<uint32_t>(begin, m_cursor);

            if (!Consume('-'))
                return false;

            // 2 digit month
            begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                 return false;

            month = String::ToInteger<uint32_t>(begin, m_cursor);

            if (!Consume('-'))
                 return false;

            // 2 digit day
            begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                 return false;

            day = String::ToInteger<uint32_t>(begin, m_cursor);

            // Check if this include time information
            if (!AtEnd() && (*m_cursor == 'T' || *m_cursor == 't' || *m_cursor == ' '))
            {
                if (!ConsumeOneOf("tT "))
                    return false;

                // 2 digit hour
                begin = m_cursor;
                if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                    return false;

                hours = String::ToInteger<uint32_t>(begin, m_cursor);

                if (!Consume(':'))
                    return false;

                // 2 digit minute
                begin = m_cursor;
                if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                    return false;

                minutes = String::ToInteger<uint32_t>(begin, m_cursor);

                // 2 digit seconds are optional in TOML
                if (!AtEnd() && *m_cursor == ':')
                {
                    if (!Consume(':'))
                            return false;

                    begin = m_cursor;
                    if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                        return false;

                    // Optional fractional seconds
                    if (!AtEnd() && *m_cursor == '.')
                    {
                        ++m_cursor;

                        if (!ConsumeOneOf(Nums))
                            return false;

                        while (!AtEnd() && IsNumeric(*m_cursor))
                            ++m_cursor;
                    }

                    seconds = String::ToFloat<double>(begin, m_cursor);
                }

                // Optional timezone
                if (!AtEnd() && (*m_cursor == 'z' || *m_cursor == 'Z'))
                {
                    ++m_cursor;
                    tzOffset = Duration_Zero;
                }
                else if (!AtEnd() && (*m_cursor == '+' || *m_cursor == '-'))
                {
                    const int64_t sign = *m_cursor == '-' ? -1 : 1;
                    ++m_cursor;

                    // 2 digit hour offset
                    begin = m_cursor;
                    if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                        return false;

                    const uint32_t tzOffsetHours = String::ToInteger<uint32_t>(begin, m_cursor);

                    if (!Consume(':'))
                        return false;

                    // 2 digit minute offset
                    begin = m_cursor;
                    if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                        return false;

                    const uint32_t tzOffsetMinutes = String::ToInteger<uint32_t>(begin, m_cursor);

                    if (tzOffsetHours > 23 || tzOffsetMinutes > 59)
                        return SetError(TomlReadError::InvalidDateTime);

                    tzOffset = FromPeriod<Hours>(sign * tzOffsetHours) + FromPeriod<Minutes>(sign * tzOffsetMinutes);
                }
                else
                {
                    // When no offset is specified we must assume local timezone
                    tzOffset = localTzOffset;
                }
            }
            else
            {
                // When there is no time information specified we must assume local timezone
                tzOffset = localTzOffset;
            }

            // Some very basic validation of the number of days allowed in a month
            const bool isLeapYear = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
            const uint32_t maxDay = (month == 2)
                ? (isLeapYear ? 29 : 28)
                : ((month == 4 || month == 6 || month == 9 || month == 11) ? 30 : 31);

            // year = [0, 9999], month = [1, 12], day = [1, maxDay]
            if (year == 0 || month < 1 || month > 12 || day < 1 || day > maxDay)
                return SetError(TomlReadError::InvalidDateTime);

            // hours = [0, 23], minutes = [0, 59], seconds = [0, 59]
            if (hours >= 23 || minutes >= 60 || seconds >= 60.0)
                return SetError(TomlReadError::InvalidDateTime);

            struct tm t{};
            t.tm_sec = static_cast<int>(seconds);
            t.tm_min = static_cast<int>(minutes);
            t.tm_hour = static_cast<int>(hours);
            t.tm_mday = static_cast<int>(day);
            t.tm_mon = static_cast<int>(month) - 1;
            t.tm_year = static_cast<int>(year) - 1900;
            t.tm_wday = 0; // the value will be ignored
            t.tm_yday = 0; // the value will be ignored
            t.tm_isdst = -1;

            const double subseconds = seconds - t.tm_sec;

            const time_t time = mktime(&t);
            if (time == -1)
                return SetError(TomlReadError::InvalidDateTime);

            SystemTime dt{ static_cast<uint64_t>(time) * Seconds::Ratio };
            dt.val += static_cast<uint64_t>(subseconds * Seconds::Ratio);

            // `mktime` gives us a value in local timezone, so we only need to modify it if the
            // timezone specified by the string is different than local. If we do need to adjust
            // we first convert the local time to UTC. Once that's done, we can offset it by the
            // timezone offset specified in the string.
            if (localTzOffset != tzOffset)
            {
                dt += localTzOffset;
                dt -= tzOffset;
            }

            m_handler->DateTime(dt);
            return true;
        }

        bool ParseNum()
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            // Check the sign
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
                return SetError(TomlReadError::UnexpectedEof);

            switch (*m_cursor)
            {
                case 'i': // inf
                {
                    const uint32_t slack = static_cast<uint32_t>(m_end - m_cursor);
                    if (slack < 3)
                        return SetError(TomlReadError::UnexpectedEof);

                    if (m_cursor[1] == 'n' && m_cursor[2] == 'f')
                    {
                        const double value = Limits<double>::Infinity;
                        m_handler->Float(isSigned ? -value : value);
                        m_cursor += 3;
                        return true;
                    }

                    return SetError(TomlReadError::InvalidToken);
                }
                case 'n': // nan
                {
                    const uint32_t slack = static_cast<uint32_t>(m_end - m_cursor);
                    if (slack < 3)
                        return SetError(TomlReadError::UnexpectedEof);

                    if (m_cursor[1] == 'a' && m_cursor[2] == 'n')
                    {
                        const double value = Limits<double>::NaN;
                        m_handler->Float(isSigned ? -value : value);
                        m_cursor += 3;
                        return true;
                    }

                    return SetError(TomlReadError::InvalidToken);
                }
                case '0': // 0x, 0o, 0b
                {
                    if (!Consume('0'))
                        return false;

                    if (AtEnd())
                    {
                        if (isSigned)
                            m_handler->Int(-0);
                        else
                            m_handler->Uint(0);
                        return true;
                    }

                    if (*m_cursor == 'x')
                    {
                        if (!Consume('x'))
                            return false;

                        return ParseHexNum(isSigned);
                    }

                    if (*m_cursor == 'o')
                    {
                        if (!Consume('o'))
                            return false;

                        return ParseOctNum(isSigned);
                    }

                    if (*m_cursor == 'b')
                    {
                        if (!Consume('b'))
                            return false;

                        return ParseBinNum(isSigned);
                    }

                    // Back up the cursor so the leading zero is restored, and fall through to
                    // decimal number parsing in the `default` case.
                    --m_cursor;
                    [[fallthrough]];
                }
                default:
                    return ParseDecNum(isSigned);
            }
        }

        bool ParseDecNum(bool isSigned)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            const char* dot = nullptr;
            const char* exp = nullptr;
            const char* expSign = nullptr;
            uint32_t dotIndex = 0;

            m_stringBuffer.Clear();

            if (isSigned)
                m_stringBuffer.PushBack('-');

            while (!AtEnd() && (IsNumeric(*m_cursor) || IsOneOf(*m_cursor, ".eE-+_")))
            {
                switch (*m_cursor)
                {
                    case '.':
                    {
                        if (!IsNumeric(m_cursor[-1]))
                            return SetError(TomlReadError::InvalidNumber);

                        if (dot)
                            return SetError(TomlReadError::InvalidNumber);

                        dot = m_cursor;
                        dotIndex = m_stringBuffer.Size();
                        break;
                    }
                    case 'e':
                    case 'E':
                    {
                        if (!IsNumeric(m_cursor[-1]))
                            return SetError(TomlReadError::InvalidNumber);

                        if (exp)
                            return SetError(TomlReadError::InvalidNumber);

                        exp = m_cursor;
                        break;
                    }
                    case '-':
                    case '+':
                    {
                        if (!exp || expSign)
                            return SetError(TomlReadError::InvalidNumber);

                        expSign = m_cursor;
                        break;
                    }
                }

                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);

                ++m_cursor;
            }

            if (m_stringBuffer.IsEmpty() || (m_stringBuffer.Size() == 1 && m_stringBuffer[0] == '-'))
                return SetError(TomlReadError::InvalidNumber);

            const char* begin = m_stringBuffer.Begin();
            if (*begin == '-')
                ++begin;

            if (dot || exp)
            {
                const char* end = m_stringBuffer.End();

                // Cannot start with a dot or exponent
                if (*begin == '.' || *begin == 'e' || *begin == 'E')
                    return SetError(TomlReadError::InvalidNumber);

                // Cannot end with a dot, exponent, or sign
                if (end[-1] == '.' || end[-1] == 'e' || end[-1] == 'E' || end[-1] == '-' || end[-1] == '+')
                    return SetError(TomlReadError::InvalidNumber);

                // Leading zeroes only allowed when followed by a decimal or exponent
                if (*begin == '0' && begin[1] != '.' && begin[1] != 'e' && begin[1] != 'E')
                    return SetError(TomlReadError::InvalidNumber);

                // exponent cannot be before the dot
                if (exp && exp < dot)
                    return SetError(TomlReadError::InvalidNumber);

                const double value = String::ToFloat<double>(m_stringBuffer.Begin(), m_stringBuffer.End());
                m_handler->Float(value);
            }
            else
            {
                // Leading zeroes are not allowed for integers
                if (*begin == '0')
                    return SetError(TomlReadError::InvalidNumber);

                if (isSigned)
                {
                    const int64_t value = String::ToInteger<int64_t>(m_stringBuffer.Begin(), m_stringBuffer.End());
                    m_handler->Int(value);
                }
                else
                {
                    const uint64_t value = String::ToInteger<uint64_t>(m_stringBuffer.Begin(), m_stringBuffer.End());
                    m_handler->Uint(value);
                }
            }

            return true;
        }

        bool ParseHexNum(bool isSigned)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            if (isSigned)
                return SetError(TomlReadError::InvalidNumber);

            m_stringBuffer.Clear();
            while (!AtEnd() && (IsHex(*m_cursor) || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            m_handler->Uint(String::ToInteger<uint64_t>(m_stringBuffer.Begin(), m_stringBuffer.End(), 16));
            return true;
        }

        bool ParseOctNum(bool isSigned)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            if (isSigned)
                return SetError(TomlReadError::InvalidNumber);

            m_stringBuffer.Clear();
            while (!AtEnd() && ((*m_cursor >= '0' && *m_cursor <= '7') || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            m_handler->Uint(String::ToInteger<uint64_t>(m_stringBuffer.Begin(), m_stringBuffer.End(), 8));
            return true;
        }

        bool ParseBinNum(bool isSigned)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            if (isSigned)
                return SetError(TomlReadError::InvalidNumber);

            m_stringBuffer.Clear();
            while (!AtEnd() && (*m_cursor == '0' || *m_cursor == '1' || *m_cursor == '_'))
            {
                if (*m_cursor != '_')
                    m_stringBuffer.PushBack(*m_cursor);
                ++m_cursor;
            }

            m_handler->Uint(String::ToInteger<uint64_t>(m_stringBuffer.Begin(), m_stringBuffer.End(), 2));
            return true;
        }

    private:
        TomlReadResult m_result{};
        TomlReader::Handler* m_handler{ nullptr };

        const char* m_cursor{ nullptr };
        const char* m_end{ nullptr };
        const char* m_lineStart{ nullptr };
        uint32_t m_line{ 1 };

        String m_stringBuffer{};
        Vector<String> m_pathBuffer{};
    };

    // --------------------------------------------------------------------------------------------
    TomlReadResult TomlReader::Read(StringView data, Handler& handler)
    {
        TomlParser parser;
        return parser.Parse(data, handler);
    }

    // --------------------------------------------------------------------------------------------
    template <>
    const char* AsString(TomlReadError x)
    {
        switch (x)
        {
            case TomlReadError::None: return "None";
            case TomlReadError::Cancelled: return "Cancelled";
            case TomlReadError::UnexpectedEof: return "UnexpectedEof";
            case TomlReadError::InvalidBom: return "InvalidBom";
            case TomlReadError::InvalidNewline: return "InvalidNewline";
            case TomlReadError::InvalidUnicode: return "InvalidUnicode";
            case TomlReadError::InvalidEscapeSequence: return "InvalidEscapeSequence";
            case TomlReadError::InvalidControlChar: return "InvalidControlChar";
            case TomlReadError::InvalidKey: return "InvalidKey";
            case TomlReadError::InvalidDateTime: return "InvalidDateTime";
            case TomlReadError::InvalidNumber: return "InvalidNumber";
            case TomlReadError::InvalidToken: return "InvalidToken";
        }

        return "<unknown>";
    }
}
