// Copyright Chad Engler

// TODO: Tests for non-english unicode sequences in bare keys, comments, strings, etc

#include "he/core/toml_reader.h"

#include "he/core/clock.h"
#include "he/core/clock_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/core/limits.h"
#include "he/core/path.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/test.h"
#include "he/core/utils.h"
#include "he/core/vector.h"
#include "he/core/vector_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
struct TomlEvent
{
    enum class Kind
    {
        DocumentStart,
        DocumentEnd,
        Comment,
        Bool,
        Int,
        Uint,
        Float,
        String,
        DateTime,
        Time,
        Table,
        Key,
        StartInlineTable,
        EndInlineTable,
        StartArray,
        EndArray,
    };

    Kind kind{ Kind::Bool };

    Vector<String> p{};
    String s{};
    bool b{ false };
    int64_t i{ 0 };
    uint64_t u{ 0 };
    double f{ 0.0 };
    SystemTime d{ 0 };
    Duration t{ 0 };

    bool operator==(const TomlEvent& x) const
    {
        if (kind != x.kind)
            return false;

        switch (kind)
        {
            case Kind::DocumentStart: return true;
            case Kind::DocumentEnd: return true;
            case Kind::Comment: return s == x.s;
            case Kind::Bool: return b == x.b;
            case Kind::Int: return i == x.i;
            case Kind::Uint: return u == x.u;
            case Kind::Float: return f == x.f;
            case Kind::String: return s == x.s;
            case Kind::DateTime: return d == x.d;
            case Kind::Time: return t == x.t;
            case Kind::Table: return p == x.p && b == x.b;
            case Kind::Key: return p == x.p;
            case Kind::StartInlineTable: return true;
            case Kind::EndInlineTable: return u == x.u;
            case Kind::StartArray: return true;
            case Kind::EndArray: return u == x.u;
        }
        return false;
    }

    bool operator!=(const TomlEvent& x) const { return !(*this == x); }
};

template <>
const char* he::AsString(TomlEvent::Kind x)
{
    switch (x)
    {
        case TomlEvent::Kind::DocumentStart: return "DocumentStart";
        case TomlEvent::Kind::DocumentEnd: return "DocumentEnd";
        case TomlEvent::Kind::Comment: return "Comment";
        case TomlEvent::Kind::Bool: return "Bool";
        case TomlEvent::Kind::Int: return "Int";
        case TomlEvent::Kind::Uint: return "Uint";
        case TomlEvent::Kind::Float: return "Float";
        case TomlEvent::Kind::String: return "String";
        case TomlEvent::Kind::DateTime: return "DateTime";
        case TomlEvent::Kind::Time: return "Time";
        case TomlEvent::Kind::Table: return "Table";
        case TomlEvent::Kind::Key: return "Key";
        case TomlEvent::Kind::StartInlineTable: return "StartInlineTable";
        case TomlEvent::Kind::EndInlineTable: return "EndInlineTable";
        case TomlEvent::Kind::StartArray: return "StartArray";
        case TomlEvent::Kind::EndArray: return "EndArray";
    }

    return "<unknown>";
}

template <>
struct he::Formatter<TomlEvent>
{
    using Type = TomlEvent;

    constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

    void Format(String& out, const TomlEvent& evt) const
    {
        switch (evt.kind)
        {
            case TomlEvent::Kind::DocumentStart:
            case TomlEvent::Kind::DocumentEnd:
            case TomlEvent::Kind::StartInlineTable:
            case TomlEvent::Kind::StartArray:
                FormatTo(out, "{}", evt.kind);
                break;

            case TomlEvent::Kind::Comment: FormatTo(out, "{} ({})", evt.kind, evt.s); break;
            case TomlEvent::Kind::Bool: FormatTo(out, "{} ({})", evt.kind, evt.b); break;
            case TomlEvent::Kind::Int: FormatTo(out, "{} ({})", evt.kind, evt.i); break;
            case TomlEvent::Kind::Uint: FormatTo(out, "{} ({})", evt.kind, evt.u); break;
            case TomlEvent::Kind::Float: FormatTo(out, "{} ({})", evt.kind, evt.f); break;
            case TomlEvent::Kind::String: FormatTo(out, "{} ({})", evt.kind, evt.s); break;
            case TomlEvent::Kind::DateTime: FormatTo(out, "{} ({})", evt.kind, evt.d); break;
            case TomlEvent::Kind::Time: FormatTo(out, "{} ({})", evt.kind, evt.t); break;
            case TomlEvent::Kind::Table: FormatTo(out, "{} ({}, {})", evt.kind, evt.p, evt.b); break;
            case TomlEvent::Kind::Key: FormatTo(out, "{} ({})", evt.kind, evt.p); break;
            case TomlEvent::Kind::EndInlineTable: FormatTo(out, "{} ({})", evt.kind, evt.u); break;
            case TomlEvent::Kind::EndArray: FormatTo(out, "{} ({})", evt.kind, evt.u); break;
        }
    }
};

// ------------------------------------------------------------------------------------------------
class TomlReaderEventHandler : public TomlReader::Handler
{
public:
    void Validate(Span<const TomlEvent> expected) const
    {
        HE_EXPECT_EQ(m_events.Size(), expected.Size());

        for (uint32_t i = 0; i < expected.Size(); ++i)
        {
            HE_EXPECT_EQ(m_events[i], expected[i]);
        }
    }

    void Reset()
    {
        m_events.Clear();
    }

private:
    bool StartDocument() override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::DocumentStart });
        return true;
    }

    bool EndDocument() override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::DocumentEnd });
        return true;
    }

    bool Comment(StringView value) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::Comment, .s = value });
        return true;
    }

    bool Bool(bool value) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::Bool, .b = value });
        return true;
    }

    bool Int(int64_t value) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::Int, .i = value });
        return true;
    }

    bool Uint(uint64_t value) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::Uint, .u = value });
        return true;
    }

    bool Float(double value) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::Float, .f = value });
        return true;
    }

    bool String(StringView value) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::String, .s = value });
        return true;
    }

    bool DateTime(SystemTime value) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::DateTime, .d = value });
        return true;
    }

    bool Time(Duration value) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::Time, .t = value });
        return true;
    }

    bool Table(Span<const he::String> path, bool isArray) override
    {
        Vector<he::String> p;
        p.Insert(0, path.Data(), path.Size());
        m_events.PushBack({ .kind = TomlEvent::Kind::Table, .p = Move(p), .b = isArray});
        return true;
    }

    bool Key(Span<const he::String> path) override
    {
        Vector<he::String> p;
        p.Insert(0, path.Data(), path.Size());
        m_events.PushBack({ .kind = TomlEvent::Kind::Key, .p = Move(p) });
        return true;
    }

    bool StartInlineTable() override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::StartInlineTable });
        return true;
    }

    bool EndInlineTable(uint32_t length) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::EndInlineTable, .u = length });
        return true;
    }

    bool StartArray() override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::StartArray });
        return true;
    }

    bool EndArray(uint32_t length) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::EndArray, .u = length });
        return true;
    }

private:
    Vector<TomlEvent> m_events;
};

// ------------------------------------------------------------------------------------------------
class TomlReaderFixture : public TestFixture
{
public:
    void Validate(StringView input, Span<const TomlEvent> events)
    {
        handler.Reset();
        const TomlReadResult result = reader.Read(input, handler);
        HE_EXPECT(result, result.error, result.line, result.column, input);
        handler.Validate(events);
    }

    void Validate(StringView input, TomlReadError expected)
    {
        handler.Reset();
        const TomlReadResult result = reader.Read(input, handler);
        HE_EXPECT(!result);
        HE_EXPECT_EQ(result.error, expected);
    }

    void ValidateKey(StringView input, StringView expectedValue)
    {
        const TomlEvent expected[] =
        {
            { .kind = TomlEvent::Kind::DocumentStart },
            { .kind = TomlEvent::Kind::Key, .p = { expectedValue } },
            { .kind = TomlEvent::Kind::Uint, .u = 123 },
            { .kind = TomlEvent::Kind::DocumentEnd },
        };

        String doc = input;
        doc += " = 123";
        Validate(doc, expected);
    }

    void ValidateValue(StringView input, const TomlEvent& expectedEvent)
    {
        const TomlEvent expected[] =
        {
            { .kind = TomlEvent::Kind::DocumentStart },
            { .kind = TomlEvent::Kind::Key, .p = { "key" } },
            expectedEvent,
            { .kind = TomlEvent::Kind::DocumentEnd },
        };

        String doc = "key = ";
        doc += input;
        Validate(doc, expected);
    }

    void ValidateBool(StringView input, bool expectedValue)
    {
        ValidateValue(input, { .kind = TomlEvent::Kind::Bool, .b = expectedValue });
    }

    void ValidateInt(StringView input, int64_t expectedValue)
    {
        ValidateValue(input, { .kind = TomlEvent::Kind::Int, .i = expectedValue });
    }

    void ValidateUint(StringView input, uint64_t expectedValue)
    {
        ValidateValue(input, { .kind = TomlEvent::Kind::Uint, .u = expectedValue });
    }

    void ValidateFloat(StringView input, double expectedValue)
    {
        ValidateValue(input, { .kind = TomlEvent::Kind::Float, .f = expectedValue });
    }

    void ValidateString(StringView input, StringView expectedValue)
    {
        ValidateValue(input, { .kind = TomlEvent::Kind::String, .s = expectedValue });
    }

    void ValidateDateTime(StringView input, SystemTime expectedValue)
    {
        ValidateValue(input, { .kind = TomlEvent::Kind::DateTime, .d = expectedValue });
    }

    void ValidateTime(StringView input, Duration expectedValue)
    {
        ValidateValue(input, { .kind = TomlEvent::Kind::Time, .t = expectedValue });
    }

    TomlReaderEventHandler handler;
    TomlReader reader;
};

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_empty, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::EndArray, .u = 0 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = []", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_spaces, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::EndArray, .u = 0 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("  \t key  =\t [ \r\n\t\t]\r\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_one_item, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 6 },
        { .kind = TomlEvent::Kind::EndArray, .u = 1},
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = [6]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_multiple_items, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Comment, .s = " this is a comment" },
        { .kind = TomlEvent::Kind::Uint, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = [1,\t2,\r\n# this is a comment\r\n\t\t3]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_multiple_heterogenous, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 0x9ffffffe },
        { .kind = TomlEvent::Kind::String, .s = "literal" },
        { .kind = TomlEvent::Kind::String, .s = "basic" },
        { .kind = TomlEvent::Kind::Float, .f = 1.5 },
        { .kind = TomlEvent::Kind::Bool, .b = true },
        { .kind = TomlEvent::Kind::String, .s = "multiline\n\n\nLiteral" },
        { .kind = TomlEvent::Kind::String, .s = "multiline\n\n\nBasic" },
        { .kind = TomlEvent::Kind::EndArray, .u = 8 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = [1, 0x9ffffffe, 'literal', \"basic\", 1.5, true, '''multiline\n\r\n\nLiteral''', \"\"\"multiline\n\r\n\nBasic\"\"\"]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_nested, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Uint, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u= 2 },
        { .kind = TomlEvent::Kind::String, .s = "4" },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::String, .s = "5" },
        { .kind = TomlEvent::Kind::String, .s = "6" },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
        { .kind = TomlEvent::Kind::EndArray, .u = 4 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = [1, [2,3], '4', [\"5\", \"6\"]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_tables, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Comment, .s = "comment!" },
        { .kind = TomlEvent::Kind::EndArray, .u = 0 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = [\n#comment!\n]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_trailing_comma, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = [1,2,]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "test" }, .b = true },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[[test]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_num, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "123" }, .b = true },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[[123]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_quoted, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "test\n123" }, .b = true },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[[\"test\\n123\"]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_nested, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "test", "123", "name with spaces" }, .b = true },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[[test.123.\"name with spaces\"]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_whitespace, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "test", "123", "name with spaces" }, .b = true },
        { .kind = TomlEvent::Kind::Comment, .s = " comment \t" },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("\t \r\n \t[[test.    123.\t\t\"name with spaces\"]] # comment \t\r\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_invalid, TomlReaderFixture)
{
    // No newlines in table names
    Validate("[[\ntest.123.\"space\\nname\"]]\n", TomlReadError::InvalidKey);
    Validate("[[test.\n123.\"space\\nname\"]]\n", TomlReadError::InvalidKey);
    Validate("[[test.123\n.\"space\\nname\"]]\n", TomlReadError::InvalidKey);

    // Multiple dots in sequence
    Validate("[[test..123.\"space\\nname\"]]\n", TomlReadError::InvalidKey);
    Validate("[[test.123..\"space\\nname\"]]\n", TomlReadError::InvalidKey);
    Validate("[[..test.123.\"space\\nname\"]]\n", TomlReadError::InvalidKey);
    Validate("[[test.123.\"space\\nname\"..]]\n", TomlReadError::InvalidKey);

    // Leading/trailing dots
    Validate("[[.test.123.\"space\\nname\"]]\n", TomlReadError::InvalidKey);
    Validate("[[test.123.\"space\\nname\".]]\n", TomlReadError::InvalidKey);
    Validate("[[.test.123.\"space\\nname\".]]\n", TomlReadError::InvalidKey);

    // At least one non-whitespace character is required
    Validate("[[  ]]\n", TomlReadError::InvalidKey);

    // Whitespace is not allowed between opening braces
    Validate("[ [name]]\n", TomlReadError::InvalidKey);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, basic_string, TomlReaderFixture)
{
    // Basic encodings
    ValidateString("\"\"", "");
    ValidateString("\"test\"", "test");
    ValidateString("\"test\\b\\t\\n\\f\\r\\\"\\\\\"", "test\b\t\n\f\r\"\\");
    ValidateString("\"\\u0001\"", "\x01");
    ValidateString("\"\\U00000001\"", "\x01");
    ValidateString("\"\\U000000e9\"", "é");
    ValidateString("\"test\\ud7fftest\"", "test\xed\x9f\xbftest");
    ValidateString("\"test\\ud7FFtest\"", "test\xed\x9f\xbftest");
    ValidateString("\"test\\ue000test\"", "test\xee\x80\x80test");
    ValidateString("\"test\\uE000test\"", "test\xee\x80\x80test");
    ValidateString("\"test\\U0000e000test\"", "test\xee\x80\x80test");
    ValidateString("\"test\\U0000E000test\"", "test\xee\x80\x80test");
    ValidateString("\"test\\U0010fffftest\"", "test\xf4\x8f\xbf\xbftest");
    ValidateString("\"test\\U0010FFFFtest\"", "test\xf4\x8f\xbf\xbftest");

    // Unescaped control characters
    Validate("key = \"test\ntest\"", TomlReadError::InvalidControlChar);
    Validate("key = \"test\0test\"", TomlReadError::UnexpectedEof);
    Validate("key = \"test\btest\"", TomlReadError::InvalidControlChar);

    // Invalid escape sequences
    Validate("key = \"test\\mtest\"", TomlReadError::InvalidEscapeSequence);
    Validate("key = \"test\\ltest\"", TomlReadError::InvalidEscapeSequence);

    // Invalid unicode sequences
    Validate("key = \"test\\ud800test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\ud900test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\udffftest\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\U0000d800test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\U0000d900test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\U0000dffftest\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\uD800test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\uD900test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\uDfFFtest\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\U0000D800test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\U0000D900test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\U0000DFFFtest\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\U00110000test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\Uaa110000test\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\u000g\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\u00GG\"", TomlReadError::InvalidUnicode);
    Validate("key = \"test\\uzzzz\"", TomlReadError::InvalidUnicode);

    // Multiline basic strings
    ValidateString("\"\"\"\"\"\"", "");
    ValidateString("\"\"\" \" \"\"\"", " \" ");
    ValidateString("\"\"\" \"\" \"\"\"", " \"\" ");
    ValidateString("\"\"\"\" \"\"\"", "\" ");
    ValidateString("\"\"\"\"\" \"\"\"", "\"\" ");
    ValidateString("\"\"\"test\"\"\"", "test");
    ValidateString("\"\"\"test\ntest\"\"\"", "test\ntest");

    // SkipFirstEOL
    ValidateString("\"\"\"\ntest\ntest\"\"\"", "test\ntest");
    ValidateString("\"\"\"\r\n   test\ntest\"\"\"", "   test\ntest");
    ValidateString("\"\"\"\n   test\r\ntest\"\"\"", "   test\ntest");

    // PreserveOtherWhitespaces
    ValidateString("\"\"\"test  \n   test \r\n   \"\"\"", "test  \n   test \n   ");

    // EscapeEOL
    ValidateString("\"\"\"test  \\\n    \r\n   \n   xxx\nyyy\r\nzzz\"\"\"", "test  xxx\nyyy\nzzz");
    ValidateString("\"\"\"test  \\\r\n    \r\n   \n   xxx\nyyy\r\nzzz\"\"\"", "test  xxx\nyyy\nzzz");

    // Escape
    ValidateString("\"\"\"test\\b\\t\\n\\f\\r\\\"\\\\\"\"\"", "test\b\t\n\f\r\"\\");
    ValidateString("\"\"\"\\u0001\"\"\"", "\x01");
    ValidateString("\"\"\"\\U00000001\"\"\"", "\x01");
    ValidateString("\"\"\"\\U000000e9\"\"\"", "é");
    ValidateString("\"\"\"test\\ud7fftest\"\"\"", "test\xed\x9f\xbftest");
    ValidateString("\"\"\"test\\ud7FFtest\"\"\"", "test\xed\x9f\xbftest");
    ValidateString("\"\"\"test\\ue000test\"\"\"", "test\xee\x80\x80test");
    ValidateString("\"\"\"test\\uE000test\"\"\"", "test\xee\x80\x80test");
    ValidateString("\"\"\"test\\U0000e000test\"\"\"", "test\xee\x80\x80test");
    ValidateString("\"\"\"test\\U0000E000test\"\"\"", "test\xee\x80\x80test");
    ValidateString("\"\"\"test\\U0010fffftest\"\"\"", "test\xf4\x8f\xbf\xbftest");
    ValidateString("\"\"\"test\\U0010FFFFtest\"\"\"", "test\xf4\x8f\xbf\xbftest");

    // Control characters must be escaped
    Validate("key = \"\"\"test\0test\"\"\"", TomlReadError::UnexpectedEof);
    Validate("key = \"\"\"test\btest\"\"\"", TomlReadError::InvalidControlChar);

    // Invalid escape sequence
    Validate("key = \"\"\"test\\mtest\"\"\"", TomlReadError::InvalidEscapeSequence);
    Validate("key = \"\"\"test\\ltest\"\"\"", TomlReadError::InvalidEscapeSequence);

    // Invalid unicode sequence
    Validate("key = \"\"\"test\\ud800test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\ud900test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\udffftest\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\U0000d800test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\U0000d900test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\U0000dffftest\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\uD800test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\uD900test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\uDfFFtest\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\U0000D800test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\U0000D900test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\U0000DFFFtest\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\U00110000test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\Uaa110000test\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\u000g\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\u00GG\"\"\"", TomlReadError::InvalidUnicode);
    Validate("key = \"\"\"test\\uzzzz\"\"\"", TomlReadError::InvalidUnicode);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, boolean_true, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "true" } },
        { .kind = TomlEvent::Kind::Bool, .b = true },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("true = true", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, boolean_false, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "false" } },
        { .kind = TomlEvent::Kind::Bool, .b = false },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("false = false", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, comments, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Comment, .s = "first comment" },
        { .kind = TomlEvent::Kind::Comment, .s = " second comment" },
        { .kind = TomlEvent::Kind::Comment, .s = "last one" },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("#first comment\n\n    \t# second comment\r\n\r\n #last one", expected);
    Validate("# this comment\x0B is erroneous", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, empty, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("", expected);
    Validate("\n\r\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, float, TomlReaderFixture)
{
    // Zeroes
    ValidateFloat("0.0", 0.0);
    ValidateFloat("+0.0", 0.0);
    ValidateFloat("-0.0", 0.0);
    ValidateFloat("0e0", 0.0);
    ValidateFloat("-0e0", 0.0);
    ValidateFloat("0e-0", 0.0);
    ValidateFloat("0e+0", 0.0);
    ValidateFloat("-0e-0", 0.0);
    ValidateFloat("-0e+0", 0.0);
    ValidateFloat("+0e-0", 0.0);
    ValidateFloat("+0e+0", 0.0);
    ValidateFloat("0e10", 0.0);
    ValidateFloat("0e-10", 0.0);
    ValidateFloat("0.0e0", 0.0);
    ValidateFloat("0.000e+0", 0.0);
    ValidateFloat("0.00e-0", 0.0);
    ValidateFloat("-0.00e+0", 0.0);
    ValidateFloat("+0.00e-100", 0.0);

    // Fractional
    ValidateFloat("+14.1345", 14.1345);
    ValidateFloat("-14.0001345", -14.0001345);
    ValidateFloat("0.0001345", 0.0001345);
    ValidateFloat("10.0", 10.0);

    // Exponential
    ValidateFloat("2e3", 2000);
    ValidateFloat("2e-3", 0.002);
    ValidateFloat("2e0", 2);
    ValidateFloat("2e+0", 2);
    ValidateFloat("2e-0", 2);
    ValidateFloat("+2e-1", 0.2);
    ValidateFloat("-2e-1", -0.2);
    ValidateFloat("1e+123", 1e123);
    ValidateFloat("1e-123", 1e-123);

    ValidateFloat("2E3", 2000);
    ValidateFloat("2E-3", 0.002);
    ValidateFloat("2E0", 2);
    ValidateFloat("2E+0", 2);
    ValidateFloat("2E-0", 2);
    ValidateFloat("+2E-1", 0.2);
    ValidateFloat("-2E-1", -0.2);
    ValidateFloat("1E+123", 1e123);
    ValidateFloat("1E-123", 1e-123);

    // FractionalExponential
    ValidateFloat("2.0e3", 2000);
    ValidateFloat("2.1e-3", 0.0021);
    ValidateFloat("2.54e0", 2.54);
    ValidateFloat("2.000e+0", 2);
    ValidateFloat("2.09e-0", 2.09);
    ValidateFloat("+2.2e-1", 0.22);
    ValidateFloat("-2.12e-1", -0.212);
    ValidateFloat("1.01e+123", 101e121);
    ValidateFloat("1.01e-123", 101e-125);
    ValidateFloat("2.0E3", 2000);
    ValidateFloat("2.1E-3", 0.0021);
    ValidateFloat("2.54E0", 2.54);
    ValidateFloat("2.000E+0", 2);
    ValidateFloat("2.09E-0", 2.09);
    ValidateFloat("+2.2E-1", 0.22);
    ValidateFloat("-2.12E-1", -0.212);
    ValidateFloat("1.01E+123", 101e121);
    ValidateFloat("1.01E-123", 101e-125);

    // Special float values
    ValidateFloat("inf", Limits<double>::Infinity);
    ValidateFloat("+inf", Limits<double>::Infinity);
    ValidateFloat("-inf", -Limits<double>::Infinity);
    ValidateFloat("nan", Limits<double>::NaN);
    ValidateFloat("+nan", Limits<double>::NaN);
    ValidateFloat("-nan", -Limits<double>::NaN);

    // Overflow
    ValidateFloat("1.0e1000", Limits<double>::Infinity);
    ValidateFloat("-1.0e1000", -Limits<double>::Infinity);

    // Leading zeros are not allowed
    Validate("key = 01.0", TomlReadError::InvalidNumber);
    Validate("key = 00.0", TomlReadError::InvalidNumber);
    Validate("key = 02e1", TomlReadError::InvalidNumber);
    Validate("key = -02e1", TomlReadError::InvalidNumber);
    Validate("key = +02e-1", TomlReadError::InvalidNumber);
    Validate("key = +02.01e-1", TomlReadError::InvalidNumber);

    // Integer part cannot be empty
    Validate("key = .1", TomlReadError::InvalidToken);
    Validate("key = .0", TomlReadError::InvalidToken);
    Validate("key = e10", TomlReadError::InvalidToken);
    Validate("key = -.1", TomlReadError::InvalidNumber);
    Validate("key = +.0", TomlReadError::InvalidNumber);

    // Fractional part cannot be empty
    Validate("key = 1.", TomlReadError::InvalidNumber);
    Validate("key = 1.e10", TomlReadError::InvalidNumber);
    Validate("key = -1.", TomlReadError::InvalidNumber);
    Validate("key = 0.", TomlReadError::InvalidNumber);
    Validate("key = +0.", TomlReadError::InvalidNumber);

    // Exponent cannot be empty
    Validate("key = 1e", TomlReadError::InvalidNumber);
    Validate("key = 1e+", TomlReadError::InvalidNumber);
    Validate("key = 1e-", TomlReadError::InvalidNumber);

    // Multiple dots are not allowed
    Validate("key = 1..0", TomlReadError::InvalidNumber);
    Validate("key = 1.0.0", TomlReadError::InvalidNumber);
    Validate("key = .25.1", TomlReadError::InvalidToken);

    // Multiple exponents are not allowed
    Validate("key = 1ee0", TomlReadError::InvalidNumber);
    Validate("key = 1e0e0", TomlReadError::InvalidNumber);
    Validate("key = e25e1", TomlReadError::InvalidToken);

    // Inf and NaN must be lowercase
    Validate("key = Inf", TomlReadError::InvalidToken);
    Validate("key = -Inf", TomlReadError::InvalidToken);
    Validate("key = INF", TomlReadError::InvalidToken);
    Validate("key = -INF", TomlReadError::InvalidToken);
    Validate("key = NAN", TomlReadError::InvalidToken);
    Validate("key = NaN", TomlReadError::InvalidToken);
    Validate("key = Nan", TomlReadError::InvalidToken);
    Validate("key = naninf", TomlReadError::InvalidToken);
    Validate("key = infnan", TomlReadError::InvalidToken);
    Validate("key = infity", TomlReadError::InvalidToken);
    Validate("key = notanum", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_empty, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 0 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = {}", expected);
    Validate("key = {\t    \t  \t\t   }", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_bare_key, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 1 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = { value = 123 }", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_num_key, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "123" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 1 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = { 123 = 123 }", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_quoted_key, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "key\nkey" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 1 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = { \"key\\nkey\" = 123 }", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_multi_key, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 456 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "123" } },
        { .kind = TomlEvent::Kind::Uint, .u = 789 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 3 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = { \"value\" = 123, value = 456, 123 = 789 }", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_with_array, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "arr" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Uint, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = { value = 123, arr=[1,\r\n2, \n\n\n\t3] }", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_with_multiline_string, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "str" } },
        { .kind = TomlEvent::Kind::String, .s = " first line\nsecond line\nanother!" },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = { value = 123, str ='''\n first line\r\nsecond line\nanother!''' }", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_multiline, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "str" } },
        { .kind = TomlEvent::Kind::String, .s = "test" },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = {\n\tvalue = 123,\r\n\tstr = \"test\",\n}", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_trailing_comma, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "str" } },
        { .kind = TomlEvent::Kind::String, .s = "test" },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = {\n\tvalue = 123,\r\n\tstr = \"test\",\n}", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, int, TomlReaderFixture)
{
    ValidateInt("-0", 0);
    ValidateInt("-1", -1);
    ValidateInt("-456789", -456789);
    ValidateInt("-9223372036854775808", Limits<int64_t>::Min);

    // No other formats supported for signed integers
    Validate("key = -0xabc", TomlReadError::InvalidNumber);
    Validate("key = -0o755", TomlReadError::InvalidNumber);
    Validate("key = -0b101", TomlReadError::InvalidNumber);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, uint, TomlReaderFixture)
{
    // TODO: underscore separators between values should be ignored

    // Decimal
    ValidateUint("0", 0);
    ValidateUint("+0", 0);
    ValidateUint("1", 1);
    ValidateUint("+1", 1);
    ValidateUint("456789", 456789);
    ValidateUint("+456789", 456789);
    ValidateUint("9223372036854775807", Limits<int64_t>::Max);
    ValidateUint("18446744073709551615", Limits<uint64_t>::Max);

    // Hex
    ValidateUint("0x0", 0);
    ValidateUint("0xdeadbeef", 0xdeadbeef);
    ValidateUint("0xffffffff", Limits<uint32_t>::Max);
    ValidateUint("0xffffffffffffffff", Limits<uint64_t>::Max);

    // Octal
    ValidateUint("0o0", 0);
    ValidateUint("0o755", 0755);
    ValidateUint("0o655", 0655);

    // Bin
    ValidateUint("0b0", 0);
    ValidateUint("0b1", 0b1);
    ValidateUint("0b11", 0b11);
    ValidateUint("0b111", 0b111);
    ValidateUint("0b1111", 0b1111);
    ValidateUint("0b1010101", 0b1010101);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, key, TomlReaderFixture)
{
    ValidateKey("key_-23_-", "key_-23_-");
    ValidateKey("123", "123");
    ValidateKey("\"key\\n\\t123\"", "key\n\t123");
    ValidateKey("\t\n\t\r\n   \tkey\t", "key");

    Validate("key\n = 123", TomlReadError::InvalidKey);
    Validate("key \n= 123", TomlReadError::InvalidKey);
    Validate("key =\n 123", TomlReadError::InvalidToken);
    Validate("key = \n123", TomlReadError::InvalidToken);
    Validate("\"key\nkey\" = 123", TomlReadError::InvalidControlChar);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, literal_string, TomlReaderFixture)
{
    ValidateString("''", "");
    ValidateString("'test'", "test");
    ValidateString("'  \t test \\test\"test'", "  \t test \\test\"test");

    ValidateString("''''''", "");
    ValidateString("''' ' '''", " ' ");
    ValidateString("''' '' '''", " '' ");
    ValidateString("'''' '''", "' ");
    ValidateString("''''' '''", "'' ");
    ValidateString("'''test'''", "test");
    ValidateString("'''test\ntest'''", "test\ntest");

    ValidateString("'''\n  test\ntest'''", "  test\ntest");
    ValidateString("'''\r\n  test\ntest'''", "  test\ntest");
    ValidateString("'''\r\ntest\r\ntest'''", "test\ntest");

    ValidateString("'''  \t test \\test\"test\\\n  test\\\r\n  test'''", "  \t test \\test\"test\\\n  test\\\n  test");

    Validate("key = '\n'", TomlReadError::InvalidControlChar);
    Validate("key = '\b'", TomlReadError::InvalidControlChar);
    Validate("key = '\x1f'", TomlReadError::InvalidControlChar);

    Validate("key = '''\b'''", TomlReadError::InvalidControlChar);
    Validate("key = '''\x1f'''", TomlReadError::InvalidControlChar);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_simple, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[key]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_num_key, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "123" }, .b = false },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[123]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_quoted_key, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "key\nkey" }, .b = false },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[\"key\\nkey\"]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_nested_keys, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "key", "123", "key\nkey" }, .b = false },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[key.123.\"key\\nkey\"]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_whitespace_key, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "key", "123", "key\nkey" }, .b = false },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[key.   123.\t  \t\"key\\nkey\"   \t  ]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_invalid_key, TomlReaderFixture)
{
    Validate("[\ntable.123.\"table\\ntable\"]", TomlReadError::InvalidKey);
    Validate("[table.\n123.\"table\\ntable\"]", TomlReadError::InvalidKey);
    Validate("[table.123\n.\"table\\ntable\"]", TomlReadError::InvalidKey);
    Validate("[]", TomlReadError::InvalidKey);
    Validate("[  ]", TomlReadError::InvalidKey);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_whitespace, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Table, .p = { "key", "123", "key\nkey" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " comment" },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("\r\n   \n  \t    [key.123.\"key\\nkey\"]   # comment\r\n\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, complex_document, TomlReaderFixture)
{
    constexpr StringView FilePath = __FILE__;
    const StringView DirName = GetDirectory(FilePath);

    String docPath = DirName;
    ConcatPath(docPath, "test_toml_document.toml");

    String tomlDoc;
    HE_EXPECT(File::ReadAll(tomlDoc, docPath.Data()));

    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        // TODO
        { .kind = TomlEvent::Kind::DocumentEnd },
    };

    Validate(tomlDoc, expected);
}
