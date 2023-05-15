// Copyright Chad Engler

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

    void ValidateKeys(StringView input, const Vector<String>& expectedValue)
    {
        const TomlEvent expected[] =
        {
            {.kind = TomlEvent::Kind::DocumentStart },
            {.kind = TomlEvent::Kind::Key, .p = expectedValue },
            {.kind = TomlEvent::Kind::Uint, .u = 123 },
            {.kind = TomlEvent::Kind::DocumentEnd },
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
HE_TEST_F(core, toml_reader, empty, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        {.kind = TomlEvent::Kind::DocumentStart },
        {.kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("", expected);
    Validate("\n\r\n", expected);
}

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
HE_TEST_F(core, toml_reader, comments, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        {.kind = TomlEvent::Kind::DocumentStart },
        {.kind = TomlEvent::Kind::Comment, .s = "first comment" },
        {.kind = TomlEvent::Kind::Comment, .s = " second comment" },
        {.kind = TomlEvent::Kind::Comment, .s = "last one" },
        {.kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("#first comment\n\n    \t# second comment\r\n\r\n #last one", expected);
    Validate("# this comment\x0B is erroneous", TomlReadError::InvalidToken);
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
HE_TEST_F(core, toml_reader, value_boolean, TomlReaderFixture)
{
    ValidateBool("true", true);
    ValidateBool("false", false);

    Validate("key = True", TomlReadError::InvalidToken);
    Validate("key = TRUE", TomlReadError::InvalidToken);
    Validate("key = False", TomlReadError::InvalidToken);
    Validate("key = FALSE", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, value_int, TomlReaderFixture)
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
HE_TEST_F(core, toml_reader, value_uint, TomlReaderFixture)
{
    // Decimal
    ValidateUint("0", 0);
    ValidateUint("+0", 0);
    ValidateUint("1", 1);
    ValidateUint("+1", 1);
    ValidateUint("456789", 456789);
    ValidateUint("+456789", 456789);
    ValidateUint("9223372036854775807", Limits<int64_t>::Max);
    ValidateUint("18446744073709551615", Limits<uint64_t>::Max);
    ValidateUint("1_000", 1000);
    ValidateUint("5_349_221", 5349221);
    ValidateUint("53_49_221", 5349221);
    ValidateUint("1_2_3_4_5", 12345);

    // Hex
    ValidateUint("0x0", 0);
    ValidateUint("0xdeadbeef", 0xdeadbeef);
    ValidateUint("0xdead_beef", 0xdeadbeef);
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
    ValidateUint("0b0101_0101", 0b01010101);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, value_float, TomlReaderFixture)
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
    Validate("key = -Inf", TomlReadError::InvalidNumber);
    Validate("key = INF", TomlReadError::InvalidToken);
    Validate("key = -INF", TomlReadError::InvalidNumber);
    Validate("key = NAN", TomlReadError::InvalidToken);
    Validate("key = NaN", TomlReadError::InvalidToken);
    Validate("key = Nan", TomlReadError::InvalidToken);
    Validate("key = naninf", TomlReadError::InvalidToken);
    Validate("key = infnan", TomlReadError::InvalidToken);
    Validate("key = infity", TomlReadError::InvalidToken);
    Validate("key = notanum", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, value_string_basic, TomlReaderFixture)
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
HE_TEST_F(core, toml_reader, value_string_literal, TomlReaderFixture)
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

    ValidateString("''''That,' she said, 'is still pointless.''''", "'That,' she said, 'is still pointless.'");
    ValidateString("'''''That,' she said, 'is still pointless.'''''", "''That,' she said, 'is still pointless.''");

    ValidateString("'''  \t test \\test\"test\\\n  test\\\r\n  test'''", "  \t test \\test\"test\\\n  test\\\n  test");

    Validate("key = '\n'", TomlReadError::InvalidControlChar);
    Validate("key = '\b'", TomlReadError::InvalidControlChar);
    Validate("key = '\x1f'", TomlReadError::InvalidControlChar);

    Validate("key = '''\b'''", TomlReadError::InvalidControlChar);
    Validate("key = '''\x1f'''", TomlReadError::InvalidControlChar);

    Validate("key = '''''That,' she said, 'is still pointless.''''''", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, value_datetime, TomlReaderFixture)
{
    // Sun, 27 May 1979 07:32:00 GMT
    constexpr SystemTime Expected{ 296638320000ull * Milliseconds::Ratio };

    // Offset Date-Time
    ValidateDateTime("1979-05-27T07:32:00Z", Expected);
    ValidateDateTime("1979-05-27T07:32:00z", Expected);
    ValidateDateTime("1979-05-27T07:32:00.999999Z", Expected + FromPeriod<Microseconds>(999999));
    ValidateDateTime("1979-05-27T07:32:00.999999z", Expected + FromPeriod<Microseconds>(999999));
    ValidateDateTime("1979-05-27T00:32:00-07:00", Expected);
    ValidateDateTime("1979-05-27T00:32:00.999999-07:00", Expected + FromPeriod<Microseconds>(999999));
    ValidateDateTime("1979-05-27 07:32:00Z", Expected);
    ValidateDateTime("1979-05-27 07:32Z", Expected);
    ValidateDateTime("1979-05-27 00:32-07:00", Expected);
    ValidateDateTime("1979-05-27t07:32:00z", Expected);
    ValidateDateTime("1979-05-27t07:32Z", Expected);
    ValidateDateTime("1979-05-27t00:32-07:00", Expected);

    // Local Date-Time
    // TODO: set the local timezone to -07:00 for these to pass
    ValidateDateTime("1979-05-27T00:32:00", Expected);
    ValidateDateTime("1979-05-27T00:32:00.999999", Expected + FromPeriod<Microseconds>(999999));
    ValidateDateTime("1979-05-27T00:32", Expected);

    // Local Date
    // TODO: set the local timezone to -07:00 for these to pass
    ValidateDateTime("1979-05-27", Expected - FromPeriod<Minutes>(32));

    // Invalid date times
    Validate("key = 1979-05-27T00:", TomlReadError::UnexpectedEof);
    Validate("key = 1979-05-27T00::", TomlReadError::InvalidToken);
    Validate("key = 1979-05-27-00:00", TomlReadError::InvalidToken);
    Validate("key = 1979:05-27", TomlReadError::InvalidToken);
    Validate("key = 1979-05:27", TomlReadError::InvalidToken);
    Validate("key = 0000-05-27", TomlReadError::InvalidDateTime);
    Validate("key = 1979-00-27", TomlReadError::InvalidDateTime);
    Validate("key = 1979-13-27", TomlReadError::InvalidDateTime);
    Validate("key = 1979-05-00", TomlReadError::InvalidDateTime);
    Validate("key = 1979-05-40", TomlReadError::InvalidDateTime);
    Validate("key = 1979-05-27T25:32:00Z", TomlReadError::InvalidDateTime);
    Validate("key = 1979-05-27T07:65:00Z", TomlReadError::InvalidDateTime);
    Validate("key = 1979-05-27T07:32:60.1Z", TomlReadError::InvalidDateTime);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, value_time, TomlReaderFixture)
{
    ValidateTime("07:32:00", FromPeriod<Hours>(7) + FromPeriod<Minutes>(32));
    ValidateTime("00:32:00.999999", FromPeriod<Minutes>(32) + FromPeriod<Microseconds>(999999));
    ValidateTime("07:32", FromPeriod<Hours>(7) + FromPeriod<Minutes>(32));
    ValidateTime("20:30:40", FromPeriod<Hours>(20) + FromPeriod<Minutes>(30) + FromPeriod<Seconds>(40));

    Validate("key = 25:32:01", TomlReadError::InvalidDateTime);
    Validate("key = 20:67", TomlReadError::InvalidDateTime);
    Validate("key = 20:30:200", TomlReadError::InvalidDateTime);
    Validate("key = 20:30:60.1", TomlReadError::InvalidDateTime);
    Validate("key = 20:30::1", TomlReadError::InvalidToken);
    Validate("key = 20:30:-10", TomlReadError::InvalidToken);
    Validate("key = -20:30:10", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, key, TomlReaderFixture)
{
    // Bare keys
    ValidateKey("key", "key");
    ValidateKey("bare_key", "bare_key");
    ValidateKey("bare-key", "bare-key");
    ValidateKey("1234", "1234");
    ValidateKey("Fuß", "Fuß");
    ValidateKey("😂", "😂");
    ValidateKey("汉语大字典", "汉语大字典");
    ValidateKey("辭源", "辭源");
    ValidateKey("பெண்டிரேம்", "பெண்டிரேம்");
    ValidateKey("key_-23_-", "key_-23_-");

    // Dotted keys
    ValidateKeys("physical.color", { "physical", "color" });
    ValidateKeys("physical.shape", { "physical", "shape" });
    ValidateKeys("site.\"google.com\"", { "site", "google.com" });
    ValidateKeys("பெண்.டிரேம்", { "பெண்", "டிரேம்" });
    ValidateKeys("3.14159", { "3", "14159" });

    // Basic string quoted keys
    ValidateKey("\"127.0.0.1\"", "127.0.0.1");
    ValidateKey("\"character encoding\"", "character encoding");
    ValidateKey("\"╠═╣\"", "╠═╣");
    ValidateKey("\"⋰∫∬∭⋱\"", "⋰∫∬∭⋱");
    ValidateKey("\"\"", "");
    ValidateKeys("fruit.\tcolor", { "fruit", "color" });
    ValidateKeys("fruit\t.\tflavor", { "fruit", "flavor" });

    // Literal string quoted keys
    ValidateKey("'quoted \"value\"'", "quoted \"value\"");
    ValidateKey("''", "");

    // Whitespace
    ValidateKey("\"key\\n\\t123\"", "key\n\t123");
    ValidateKey("\t\n\t\r\n   \tkey\t", "key");

    // Invalid keys
    Validate("= 123", TomlReadError::InvalidKey);
    Validate("\"\"\"key\"\"\" = 123", TomlReadError::InvalidToken);
    Validate("key\n = 123", TomlReadError::InvalidKey);
    Validate("key \n= 123", TomlReadError::InvalidKey);
    Validate("key =\n 123", TomlReadError::InvalidToken);
    Validate("key = \n123", TomlReadError::InvalidToken);
    Validate("\"key\nkey\" = 123", TomlReadError::InvalidControlChar);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, complex_document, TomlReaderFixture)
{
    // Sun, 27 May 1979 07:32:00 GMT
    constexpr SystemTime ExpectedDateTime{ 296638320000ull * Milliseconds::Ratio };

    // TODO: This only works if build in the same place it runs. Need a better approach.
    constexpr StringView FilePath = __FILE__;
    const StringView DirName = GetDirectory(FilePath);

    String docPath = DirName;
    ConcatPath(docPath, "test_toml_document.toml");

    String tomlDoc;
    HE_EXPECT(File::ReadAll(tomlDoc, docPath.Data()));

    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Comment, .s = " Copyright Chad Engler" },
        { .kind = TomlEvent::Kind::Table, .p = { "boolean" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "boolean", "bool1" } },
        { .kind = TomlEvent::Kind::Bool, .b = true },
        { .kind = TomlEvent::Kind::Key, .p = { "boolean", "bool2" } },
        { .kind = TomlEvent::Kind::Bool, .b = false },
        { .kind = TomlEvent::Kind::Table, .p = { "integer" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "int1" } },
        { .kind = TomlEvent::Kind::Uint, .u = 99 },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "int2" } },
        { .kind = TomlEvent::Kind::Uint, .u = 42 },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "int3" } },
        { .kind = TomlEvent::Kind::Uint, .u = 0 },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "int4" } },
        { .kind = TomlEvent::Kind::Int, .i = -17 },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "int5" } },
        { .kind = TomlEvent::Kind::Uint, .u = 1000 },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "int6" } },
        { .kind = TomlEvent::Kind::Uint, .u = 5349221 },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "int7" } },
        { .kind = TomlEvent::Kind::Uint, .u = 5349221 },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "int8" } },
        { .kind = TomlEvent::Kind::Uint, .u = 12345 },
        { .kind = TomlEvent::Kind::Comment, .s = " valid but inadvisable" },
        { .kind = TomlEvent::Kind::Comment, .s = " hexadecimal with prefix `0x`" },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "hex1" } },
        { .kind = TomlEvent::Kind::Uint, .u = 0xDEADBEEF },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "hex2" } },
        { .kind = TomlEvent::Kind::Uint, .u = 0xdeadbeef },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "hex3" } },
        { .kind = TomlEvent::Kind::Uint, .u = 0xdeadbeef },
        { .kind = TomlEvent::Kind::Comment, .s = " octal with prefix `0o`" },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "oct1" } },
        { .kind = TomlEvent::Kind::Uint, .u = 01234567 },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "oct2" } },
        { .kind = TomlEvent::Kind::Uint, .u = 0755 },
        { .kind = TomlEvent::Kind::Comment, .s = " useful for Unix file permissions" },
        { .kind = TomlEvent::Kind::Comment, .s = " binary with prefix `0b`" },
        { .kind = TomlEvent::Kind::Key, .p = { "integer", "bin1" } },
        { .kind = TomlEvent::Kind::Uint, .u = 0b11010110 },
        { .kind = TomlEvent::Kind::Table, .p = { "float" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " fractional" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "flt1" } },
        { .kind = TomlEvent::Kind::Float, .f = 1.0 },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "flt2" } },
        { .kind = TomlEvent::Kind::Float, .f = 3.1415 },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "flt3" } },
        { .kind = TomlEvent::Kind::Float, .f = -0.01 },
        { .kind = TomlEvent::Kind::Comment, .s = " exponent" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "flt4" } },
        { .kind = TomlEvent::Kind::Float, .f = 5e+22 },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "flt5" } },
        { .kind = TomlEvent::Kind::Float, .f = 1e06 },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "flt6" } },
        { .kind = TomlEvent::Kind::Float, .f = -2E-2 },
        { .kind = TomlEvent::Kind::Comment, .s = " both" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "flt7" } },
        { .kind = TomlEvent::Kind::Float, .f = 6.626e-34 },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "flt8" } },
        { .kind = TomlEvent::Kind::Float, .f = 224617.445991228 },
        { .kind = TomlEvent::Kind::Comment, .s = " infinity" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "sf1" } },
        { .kind = TomlEvent::Kind::Float, .f = Limits<double>::Infinity },
        { .kind = TomlEvent::Kind::Comment, .s = " positive infinity" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "sf2" } },
        { .kind = TomlEvent::Kind::Float, .f = Limits<double>::Infinity },
        { .kind = TomlEvent::Kind::Comment, .s = " positive infinity" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "sf3" } },
        { .kind = TomlEvent::Kind::Float, .f = -Limits<double>::Infinity },
        { .kind = TomlEvent::Kind::Comment, .s = " negative infinity" },
        { .kind = TomlEvent::Kind::Comment, .s = " not a number" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "sf4" } },
        { .kind = TomlEvent::Kind::Float, .f = Limits<double>::NaN },
        { .kind = TomlEvent::Kind::Comment, .s = " actual sNaN/qNaN encoding is implementation-specific" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "sf5" } },
        { .kind = TomlEvent::Kind::Float, .f = Limits<double>::NaN },
        { .kind = TomlEvent::Kind::Comment, .s = " same as `nan`" },
        { .kind = TomlEvent::Kind::Key, .p = { "float", "sf6" } },
        { .kind = TomlEvent::Kind::Float, .f = -Limits<double>::NaN },
        { .kind = TomlEvent::Kind::Comment, .s = " valid, actual encoding is implementation-specific" },
        { .kind = TomlEvent::Kind::Table, .p = { "string" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str" } },
        { .kind = TomlEvent::Kind::String, .s = "I'm a string. \"You can quote me\". Name\tJos\xE9\nLocation\tSF." },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str1" } },
        { .kind = TomlEvent::Kind::String, .s = "Roses are red\nViolets are blue" },
        { .kind = TomlEvent::Kind::Comment, .s = " On a Unix system, the above multi-line string will most likely be the same as:" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str2" } },
        { .kind = TomlEvent::Kind::String, .s = "Roses are red\nViolets are blue" },
        { .kind = TomlEvent::Kind::Comment, .s = " On a Windows system, it will most likely be equivalent to:" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str3" } },
        { .kind = TomlEvent::Kind::String, .s = "Roses are red\r\nViolets are blue" },
        { .kind = TomlEvent::Kind::Comment, .s = " The following strings are byte-for-byte equivalent:" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str4" } },
        { .kind = TomlEvent::Kind::String, .s = "The quick brown fox jumps over the lazy dog." },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str5" } },
        { .kind = TomlEvent::Kind::String, .s = "The quick brown fox jumps over the lazy dog." },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str6" } },
        { .kind = TomlEvent::Kind::String, .s = "The quick brown fox jumps over the lazy dog." },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str7" } },
        { .kind = TomlEvent::Kind::String, .s = "Here are two quotation marks: \"\". Simple enough." },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str8" } },
        { .kind = TomlEvent::Kind::String, .s = "Here are three quotation marks: \"\"\"." },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str9" } },
        { .kind = TomlEvent::Kind::String, .s = "Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\"." },
        { .kind = TomlEvent::Kind::Comment, .s = " What you see is what you get." },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "winpath" } },
        { .kind = TomlEvent::Kind::String, .s = "C:\\Users\\nodejs\\templates" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "winpath2" } },
        { .kind = TomlEvent::Kind::String, .s = "\\\\ServerX\\admin$\\system32\\" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "quoted" } },
        { .kind = TomlEvent::Kind::String, .s = "Tom \"Dubs\" Preston-Werner" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "regex" } },
        { .kind = TomlEvent::Kind::String, .s = "<\\i\\c*\\s*>" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "regex2" } },
        { .kind = TomlEvent::Kind::String, .s = "I [dw]on't need \\d{2} apples" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "lines" } },
        { .kind = TomlEvent::Kind::String, .s = "The first newline is\ntrimmed in literal strings.\n   All other whitespace\n   is preserved.\n" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "quot15" } },
        { .kind = TomlEvent::Kind::String, .s = "Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\"" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "apos15" } },
        { .kind = TomlEvent::Kind::String, .s = "Here are fifteen apostrophes: '''''''''''''''" },
        { .kind = TomlEvent::Kind::Comment, .s = " 'That,' she said, 'is still pointless.'" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str11" } },
        { .kind = TomlEvent::Kind::String, .s = "'That,' she said, 'is still pointless.'" },
        { .kind = TomlEvent::Kind::Key, .p = { "string", "str12" } },
        { .kind = TomlEvent::Kind::String, .s = "'That,' she said, 'is still pointless.''" },
        // TODO: set the local timezone to -07:00 for these to pass
        { .kind = TomlEvent::Kind::Table, .p = { "datetime" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "odt1" } },
        { .kind = TomlEvent::Kind::DateTime, .d = ExpectedDateTime },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "odt2" } },
        { .kind = TomlEvent::Kind::DateTime, .d = ExpectedDateTime },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "odt3" } },
        { .kind = TomlEvent::Kind::DateTime, .d = (ExpectedDateTime + FromPeriod<Microseconds>(999999)) },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "odt4" } },
        { .kind = TomlEvent::Kind::DateTime, .d = ExpectedDateTime },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "odt5" } },
        { .kind = TomlEvent::Kind::DateTime, .d = ExpectedDateTime },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "odt6" } },
        { .kind = TomlEvent::Kind::DateTime, .d = ExpectedDateTime },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "ldt1" } },
        { .kind = TomlEvent::Kind::DateTime, .d = ExpectedDateTime },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "ldt2" } },
        { .kind = TomlEvent::Kind::DateTime, .d = (ExpectedDateTime + FromPeriod<Microseconds>(999999)) },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "ldt3" } },
        { .kind = TomlEvent::Kind::DateTime, .d = ExpectedDateTime },
        { .kind = TomlEvent::Kind::Key, .p = { "datetime", "ld1" } },
        { .kind = TomlEvent::Kind::DateTime, .d = (ExpectedDateTime - FromPeriod<Minutes>(32)) },
        { .kind = TomlEvent::Kind::Table, .p = { "time" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "time", "lt1" } },
        { .kind = TomlEvent::Kind::Time, .t = (FromPeriod<Hours>(7) + FromPeriod<Minutes>(32)) },
        { .kind = TomlEvent::Kind::Key, .p = { "time", "lt2" } },
        { .kind = TomlEvent::Kind::Time, .t = (FromPeriod<Minutes>(32) + FromPeriod<Microseconds>(999999)) },
        { .kind = TomlEvent::Kind::Key, .p = { "time", "lt3" } },
        { .kind = TomlEvent::Kind::Time, .t = (FromPeriod<Hours>(7) + FromPeriod<Minutes>(32)) },
        { .kind = TomlEvent::Kind::Table, .p = { "array" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "integers" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Uint, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "colors" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::String, .s = "red" },
        { .kind = TomlEvent::Kind::String, .s = "yellow" },
        { .kind = TomlEvent::Kind::String, .s = "green" },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "nested_arrays_of_ints" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 3 },
        { .kind = TomlEvent::Kind::Uint, .u = 4 },
        { .kind = TomlEvent::Kind::Uint, .u = 5 },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "nested_mixed_array" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::String, .s = "a" },
        { .kind = TomlEvent::Kind::String, .s = "b" },
        { .kind = TomlEvent::Kind::String, .s = "c" },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "string_array" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::String, .s = "all" },
        { .kind = TomlEvent::Kind::String, .s = "strings" },
        { .kind = TomlEvent::Kind::String, .s = "are the same" },
        { .kind = TomlEvent::Kind::String, .s = "type" },
        { .kind = TomlEvent::Kind::EndArray, .u = 4 },
        { .kind = TomlEvent::Kind::Comment, .s = " Mixed-type arrays are allowed" },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "numbers" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Float, .f = 0.1 },
        { .kind = TomlEvent::Kind::Float, .f = 0.2 },
        { .kind = TomlEvent::Kind::Float, .f = 0.5 },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Uint, .u = 5 },
        { .kind = TomlEvent::Kind::EndArray, .u = 6 },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "contributors" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::String, .s = "Foo Bar <foo@example.com>" },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "contributors", "name" } },
        { .kind = TomlEvent::Kind::String, .s = "Baz Qux" },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "contributors", "email" } },
        { .kind = TomlEvent::Kind::String, .s = "bazqux@example.com" },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "contributors", "url" } },
        { .kind = TomlEvent::Kind::String, .s = "https://example.com/bazqux" },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "integers2" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Uint, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::Key, .p = { "array", "integers3" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Comment, .s = " this is ok" },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
        { .kind = TomlEvent::Kind::Table, .p = { "table" }, .b = false },
        { .kind = TomlEvent::Kind::Table, .p = { "table-1" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "table-1", "key1" } },
        { .kind = TomlEvent::Kind::String, .s = "some string" },
        { .kind = TomlEvent::Kind::Key, .p = { "table-1", "key2" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Table, .p = { "table-2" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "table-2", "key1" } },
        { .kind = TomlEvent::Kind::String, .s = "another string" },
        { .kind = TomlEvent::Kind::Key, .p = { "table-2", "key2" } },
        { .kind = TomlEvent::Kind::Uint, .u = 456 },
        { .kind = TomlEvent::Kind::Table, .p = { "dog", "tater.man" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "dog", "tater.man", "type", "name" } },
        { .kind = TomlEvent::Kind::String, .s = "pug" },
        { .kind = TomlEvent::Kind::Table, .p = { "a", "b", "c" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " this is best practice" },
        { .kind = TomlEvent::Kind::Table, .p = { "d", "e", "f" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " same as [d.e.f]" },
        { .kind = TomlEvent::Kind::Table, .p = { "g", "h", "i" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " same as [g.h.i]" },
        { .kind = TomlEvent::Kind::Table, .p = { "j", "ʞ", "l" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " same as [j.\"ʞ\".'l']" },
        { .kind = TomlEvent::Kind::Comment, .s = " [x] you" },
        { .kind = TomlEvent::Kind::Comment, .s = " [x.y] don't" },
        { .kind = TomlEvent::Kind::Comment, .s = " [x.y.z] need these" },
        { .kind = TomlEvent::Kind::Table, .p = { "x", "y", "z", "w" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " for this to work" },
        { .kind = TomlEvent::Kind::Table, .p = { "x" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " defining a super-table afterward is ok" },
        { .kind = TomlEvent::Kind::Table, .p = { "fruit" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "fruit", "apple", "color" } },
        { .kind = TomlEvent::Kind::String, .s = "red" },
        { .kind = TomlEvent::Kind::Key, .p = { "fruit", "apple", "taste", "sweet" } },
        { .kind = TomlEvent::Kind::Bool, .b = true },
        { .kind = TomlEvent::Kind::Table, .p = { "fruit", "apple", "texture" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " you can add sub-tables" },
        { .kind = TomlEvent::Kind::Key, .p = { "fruit", "apple", "texture", "smooth" } },
        { .kind = TomlEvent::Kind::Bool, .b = true },
        { .kind = TomlEvent::Kind::Table, .p = { "inline", "table" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "name" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "name", "first" } },
        { .kind = TomlEvent::Kind::String, .s = "Tom" },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "name", "last" } },
        { .kind = TomlEvent::Kind::String, .s = "Preston-Werner" },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "point" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "point", "x" } },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "point", "y" } },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "animal" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "animal", "type", "name" } },
        { .kind = TomlEvent::Kind::String, .s = "pug" },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 1 },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "contact" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "contact", "personal" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "contact", "personal", "name" } },
        { .kind = TomlEvent::Kind::String, .s = "Donald Duck" },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "contact", "personal", "email" } },
        { .kind = TomlEvent::Kind::String, .s = "donald@duckburg.com" },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "contact", "work" } },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "contact", "work", "name" } },
        { .kind = TomlEvent::Kind::String, .s = "Coin cleaner" },
        { .kind = TomlEvent::Kind::Key, .p = { "inline", "table", "contact", "work", "email" } },
        { .kind = TomlEvent::Kind::String, .s = "donald@ScroogeCorp.com" },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 2 },
        { .kind = TomlEvent::Kind::Table, .p = { "product" }, .b = true },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "name" } },
        { .kind = TomlEvent::Kind::String, .s = "Hammer" },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "sku" } },
        { .kind = TomlEvent::Kind::Uint, .u = 738594937 },
        { .kind = TomlEvent::Kind::Table, .p = { "product" }, .b = true },
        { .kind = TomlEvent::Kind::Comment, .s = " empty table within the array" },
        { .kind = TomlEvent::Kind::Table, .p = { "product" }, .b = true },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "name" } },
        { .kind = TomlEvent::Kind::String, .s = "Nail" },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "sku" } },
        { .kind = TomlEvent::Kind::Uint, .u = 284758393 },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "color" } },
        { .kind = TomlEvent::Kind::String, .s = "gray" },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "x" } },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "y" } },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "z" } },
        { .kind = TomlEvent::Kind::Uint, .u = 3 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 3 },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "x" } },
        { .kind = TomlEvent::Kind::Uint, .u = 7 },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "y" } },
        { .kind = TomlEvent::Kind::Uint, .u = 8 },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "z" } },
        { .kind = TomlEvent::Kind::Uint, .u = 9 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 3 },
        { .kind = TomlEvent::Kind::StartInlineTable },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "x" } },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "y" } },
        { .kind = TomlEvent::Kind::Uint, .u = 4 },
        { .kind = TomlEvent::Kind::Key, .p = { "product", "points", "z" } },
        { .kind = TomlEvent::Kind::Uint, .u = 8 },
        { .kind = TomlEvent::Kind::EndInlineTable, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };

    Validate(tomlDoc, expected);
}
