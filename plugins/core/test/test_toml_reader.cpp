// Copyright Chad Engler

#include "he/core/toml_reader.h"

#include "he/core/enum_ops.h"
#include "he/core/limits.h"
#include "he/core/string.h"
#include "he/core/test.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

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
        StartTable,
        Key,
        EndTable,
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
            case Kind::Key: return p == x.p;
            case Kind::StartTable: return p == x.p && b == x.b;
            case Kind::EndTable: return u == x.u;
            case Kind::StartArray: return true;
            case Kind::EndArray: return u == x.u;
        }
        return false;
    }

    bool operator!=(const TomlEvent& x) const { return !(*this == x); }
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
            HE_EXPECT(m_events[i] == expected[i]);
        }
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

    bool StartTable(Span<const StringView> path, bool isArray) override
    {
        Vector<he::String> p;
        for (auto& s : path)
            p.EmplaceBack(s);
        m_events.PushBack({ .kind = TomlEvent::Kind::StartTable, .p = Move(p), .b = isArray});
        return true;
    }

    bool Key(Span<const StringView> path) override
    {
        Vector<he::String> p;
        for (auto& s : path)
            p.EmplaceBack(s);
        m_events.PushBack({ .kind = TomlEvent::Kind::Key, .p = Move(p) });
        return true;
    }

    bool EndTable(uint32_t keyCount) override
    {
        m_events.PushBack({ .kind = TomlEvent::Kind::EndTable, .u = keyCount });
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
        const TomlReadResult result = reader.Read(input, handler);
        HE_EXPECT(result);
        handler.Validate(events);
    }

    void Validate(StringView input, TomlReadError expected)
    {
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

    void ValidateString(StringView input, StringView expectedValue)
    {
        const TomlEvent expected[] =
        {
            { .kind = TomlEvent::Kind::DocumentStart },
            { .kind = TomlEvent::Kind::Key, .p = { "key" } },
            { .kind = TomlEvent::Kind::String, .s = expectedValue },
            { .kind = TomlEvent::Kind::DocumentEnd },
        };

        String doc = "key = ";
        doc += input;
        Validate(doc, expected);
    }

    void ValidateFloat(StringView input, double expectedValue)
    {
        const TomlEvent expected[] =
        {
            { .kind = TomlEvent::Kind::DocumentStart },
            { .kind = TomlEvent::Kind::Key, .p = { "key" } },
            { .kind = TomlEvent::Kind::Float, .f = expectedValue },
            { .kind = TomlEvent::Kind::DocumentEnd },
        };

        String doc = "key = ";
        doc += input;
        Validate(doc, expected);
    }

    void ValidateInt(StringView input, int64_t expectedValue)
    {
        const TomlEvent expected[] =
        {
            { .kind = TomlEvent::Kind::DocumentStart },
            { .kind = TomlEvent::Kind::Key, .p = { "key" } },
            { .kind = TomlEvent::Kind::Int, .i = expectedValue },
            { .kind = TomlEvent::Kind::DocumentEnd },
        };

        String doc = "key = ";
        doc += input;
        Validate(doc, expected);
    }

    void ValidateUint(StringView input, uint64_t expectedValue)
    {
        const TomlEvent expected[] =
        {
            { .kind = TomlEvent::Kind::DocumentStart },
            { .kind = TomlEvent::Kind::Key, .p = { "key" } },
            { .kind = TomlEvent::Kind::Uint, .u = expectedValue },
            { .kind = TomlEvent::Kind::DocumentEnd },
        };

        String doc = "key = ";
        doc += input;
        Validate(doc, expected);
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
        { .kind = TomlEvent::Kind::Int,.i = 6 },
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
        { .kind = TomlEvent::Kind::Int, .i = 1 },
        { .kind = TomlEvent::Kind::Int, .i = 2 },
        { .kind = TomlEvent::Kind::Comment, .s = " this is a comment" },
        { .kind = TomlEvent::Kind::Int, .i = 3 },
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
        { .kind = TomlEvent::Kind::Int, .i = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 0x9ffffffe },
        { .kind = TomlEvent::Kind::String, .s = "literal" },
        { .kind = TomlEvent::Kind::String, .s = "basic" },
        { .kind = TomlEvent::Kind::Float, .f = 1.5 },
        { .kind = TomlEvent::Kind::Bool, .b = true },
        { .kind = TomlEvent::Kind::String, .s = "multiline\n\r\n\nLiteral" },
        { .kind = TomlEvent::Kind::String, .s = "multiline\n\r\n\nBasic" },
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
        { .kind = TomlEvent::Kind::Int, .i = 1 },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Int, .i = 2 },
        { .kind = TomlEvent::Kind::Int, .i = 3 },
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
        { .kind = TomlEvent::Kind::Int, .i = 1 },
        { .kind = TomlEvent::Kind::Int, .i = 2 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "test" }, .b = true },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "123" }, .b = true },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "test\n123" }, .b = true },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "test", "123", "name with spaces" }, .b = true },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "test", "123", "name with spaces" }, .b = true },
        { .kind = TomlEvent::Kind::Comment, .s = " comment" },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("\t \r\n \t[[test.    123.\t\t\"name with spaces\"]] # comment \t\r\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_invalid, TomlReaderFixture)
{
    // No newlines in table names
    Validate("[[\ntest.123.\"space\\nname\"]]\n", TomlReadError::InvalidToken);
    Validate("[[test.\n123.\"space\\nname\"]]\n", TomlReadError::InvalidToken);
    Validate("[[test.123\n.\"space\\nname\"]]\n", TomlReadError::InvalidToken);

    // At least one non-whitespace character is required
    Validate("[[  ]]\n", TomlReadError::InvalidToken);

    // Whitespace is not allowed between opening braces
    Validate("[ [  ]]\n", TomlReadError::InvalidToken);
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
    Validate("key = \"test\ntest\"", TomlReadError::InvalidToken);
    Validate("key = \"test\0test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\btest\"", TomlReadError::InvalidToken);

    // Invalid escape sequences
    Validate("key = \"test\\mtest\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\xtest\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\ltest\"", TomlReadError::InvalidToken);

    // Invalid unicode sequences
    Validate("key = \"test\\ud800test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\ud900test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\udffftest\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\U0000d800test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\U0000d900test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\U0000dffftest\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\uD800test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\uD900test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\uDfFFtest\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\U0000D800test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\U0000D900test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\U0000DFFFtest\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\U00110000test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\Uaa110000test\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\u000g\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\u00GG\"", TomlReadError::InvalidToken);
    Validate("key = \"test\\uzzzz\"", TomlReadError::InvalidToken);

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
    Validate("\"\"\"test\0test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\btest\"\"\"", TomlReadError::InvalidToken);

    // Invalid escape sequence
    Validate("\"\"\"test\\mtest\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\xtest\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\ltest\"\"\"", TomlReadError::InvalidToken);

    // Invalid unicode sequence
    Validate("\"\"\"test\\ud800test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\ud900test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\udffftest\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\U0000d800test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\U0000d900test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\U0000dffftest\"\"\"", TomlReadError::InvalidToken);

    Validate("\"\"\"test\\uD800test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\uD900test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\uDfFFtest\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\U0000D800test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\U0000D900test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\U0000DFFFtest\"\"\"", TomlReadError::InvalidToken);

    Validate("\"\"\"test\\U00110000test\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\Uaa110000test\"\"\"", TomlReadError::InvalidToken);

    Validate("\"\"\"test\\u000g\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\u00GG\"\"\"", TomlReadError::InvalidToken);
    Validate("\"\"\"test\\uzzzz\"\"\"", TomlReadError::InvalidToken);
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
    Validate("# this comment\b is erroneous", TomlReadError::InvalidToken);
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

    // Leading zeros are not allowed
    Validate("key = 01.0", TomlReadError::InvalidToken);
    Validate("key = 00.0", TomlReadError::InvalidToken);
    Validate("key = 02e1", TomlReadError::InvalidToken);
    Validate("key = -02e1", TomlReadError::InvalidToken);
    Validate("key = +02e-1", TomlReadError::InvalidToken);
    Validate("key = +02.01e-1", TomlReadError::InvalidToken);

    // Leading zeros are not allowed in exponent
    Validate("key = 1e01", TomlReadError::InvalidToken);
    Validate("key = 1e-01", TomlReadError::InvalidToken);
    Validate("key = 1.1e+01", TomlReadError::InvalidToken);
    Validate("key = -1.1e-00", TomlReadError::InvalidToken);

    // Integer part cannot be empty
    Validate("key = .1", TomlReadError::InvalidToken);
    Validate("key = e10", TomlReadError::InvalidToken);
    Validate("key = -.1", TomlReadError::InvalidToken);
    Validate("key = .0", TomlReadError::InvalidToken);
    Validate("key = +.0", TomlReadError::InvalidToken);

    // Fractional part cannot be empty
    Validate("key = 1.", TomlReadError::InvalidToken);
    Validate("key = 1.e10", TomlReadError::InvalidToken);
    Validate("key = -1.", TomlReadError::InvalidToken);
    Validate("key = 0.", TomlReadError::InvalidToken);
    Validate("key = +0.", TomlReadError::InvalidToken);

    // Exponent cannot be empty
    Validate("key = 1e", TomlReadError::InvalidToken);
    Validate("key = 1e+", TomlReadError::InvalidToken);
    Validate("key = 1e-", TomlReadError::InvalidToken);

    // Overflow
    Validate("key = 1.0e1000", TomlReadError::InvalidToken);
    Validate("key = -1.0e1000", TomlReadError::InvalidToken);

    // Inf and NaN are not supported
    Validate("key = Inf", TomlReadError::InvalidToken);
    Validate("key = -Inf", TomlReadError::InvalidToken);
    Validate("key = inf", TomlReadError::InvalidToken);
    Validate("key = -inf", TomlReadError::InvalidToken);
    Validate("key = INF", TomlReadError::InvalidToken);
    Validate("key = -INF", TomlReadError::InvalidToken);
    Validate("key = NAN", TomlReadError::InvalidToken);
    Validate("key = NaN", TomlReadError::InvalidToken);
    Validate("key = Nan", TomlReadError::InvalidToken);
    Validate("key = nan", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table_empty, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::EndTable, .u = 1 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "123" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::EndTable, .u = 1 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "key\nkey" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::EndTable, .u = 1 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 456 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "123" } },
        { .kind = TomlEvent::Kind::Uint, .u = 789 },
        { .kind = TomlEvent::Kind::EndTable, .u = 3 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "arr" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Uint, .u = 1 },
        { .kind = TomlEvent::Kind::Uint, .u = 2 },
        { .kind = TomlEvent::Kind::Uint, .u = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
        { .kind = TomlEvent::Kind::EndTable, .u = 1 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "str" } },
        { .kind = TomlEvent::Kind::String, .s = " first line\r\nsecond line\nanother!" },
        { .kind = TomlEvent::Kind::EndTable, .u = 2 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "str" } },
        { .kind = TomlEvent::Kind::String, .s = "test" },
        { .kind = TomlEvent::Kind::EndTable, .u = 2 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "value" } },
        { .kind = TomlEvent::Kind::Uint, .u = 123 },
        { .kind = TomlEvent::Kind::Key, .p = { "key", "str" } },
        { .kind = TomlEvent::Kind::String, .s = "test" },
        { .kind = TomlEvent::Kind::EndTable, .u = 2 },
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
    Validate("key = -0xabc", TomlReadError::InvalidToken);
    Validate("key = -0o755", TomlReadError::InvalidToken);
    Validate("key = -0b101", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, uint, TomlReaderFixture)
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

    Validate("key\n = 123", TomlReadError::InvalidToken);
    Validate("key \n= 123", TomlReadError::InvalidToken);
    Validate("key =\n 123", TomlReadError::InvalidToken);
    Validate("key = \n123", TomlReadError::InvalidToken);
    Validate("\"key\nkey\" = 123", TomlReadError::InvalidToken);
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

    Validate("'\n'", TomlReadError::InvalidToken);
    Validate("'\b'", TomlReadError::InvalidToken);
    Validate("'\x1f'", TomlReadError::InvalidToken);

    Validate("'''\b'''", TomlReadError::InvalidToken);
    Validate("'''\x1f'''", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_simple, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::StartTable, .p = { "key" }, .b = false },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "123" }, .b = false },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key\nkey" }, .b = false },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key", "123", "key\nkey" }, .b = false },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
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
        { .kind = TomlEvent::Kind::StartTable, .p = { "key", "123", "key\nkey" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " comment" },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("[key.   123.\t  \t\"key\\nkey\"   \t  ]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_invalid_key, TomlReaderFixture)
{
    Validate("[\ntable.123.\"table\\ntable\"]", TomlReadError::InvalidToken);
    Validate("[table.\n123.\"table\\ntable\"]", TomlReadError::InvalidToken);
    Validate("[table.123\n.\"table\\ntable\"]", TomlReadError::InvalidToken);
    Validate("[]", TomlReadError::InvalidToken);
    Validate("[  ]", TomlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table_whitespace, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        { .kind = TomlEvent::Kind::StartTable, .p = { "key", "123", "key\nkey" }, .b = false },
        { .kind = TomlEvent::Kind::Comment, .s = " comment" },
        { .kind = TomlEvent::Kind::EndTable, .u = 0 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("\r\n   \n  \t    [key.123.\"key\\nkey\"]   # comment\r\n\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, complex_document, TomlReaderFixture)
{
    constexpr StringView ComplexDocument = R"(
################################################################################
## Comment

# Speak your mind with the hash symbol. They go from the symbol to the end of
# the line.

key1 = 1323
228 = 228

################################################################################
## Table

# Tables (also known as hash tables or dictionaries) are collections of
# key/value pairs. They appear in square brackets on a line by themselves.

[table]

key = "value" # Yeah, you can do this.

# Nested tables are denoted by table names with dots in them. Name your tables
# whatever crap you please, just don't use #, ., [ or ].

[table.subtable]

key = "another value"

# You don't need to specify all the super-tables if you don't want to. TOML
# knows how to do it for you.

# [x] you
# [x.y] don't
# [x.y.z] need these
[x.y.z.w] # for this to work


################################################################################
## Inline Table

# Inline tables provide a more compact syntax for expressing tables. They are
# especially useful for grouped data that can otherwise quickly become verbose.
# Inline tables are enclosed in curly braces `{` and `}`. No newlines are
# allowed between the curly braces unless they are valid within a value.

[table.inline]

name = { first = "Tom", last = "Preston-Werner" }
point = { x = 1, y = 2 }


################################################################################
## String

# There are four ways to express strings: basic, multi-line basic, literal, and
# multi-line literal. All strings must contain only valid UTF-8 characters.

[string.basic]

basic = "I'm a string. \"You can quote me\". Name\tJos\u00E9\nLocation\tSF."

[string.multiline]

# The following strings are byte-for-byte equivalent:
key1 = "One\nTwo"
key2 = """One\nTwo"""
key3 = """
One
Two"""

[string.multiline.continued]

# The following strings are byte-for-byte equivalent:
key1 = "The quick brown fox jumps over the lazy dog."

key2 = """
The quick brown \


  fox jumps over \
    the lazy dog."""

key3 = """\
       The quick brown \
       fox jumps over \
       the lazy dog.\
       """

[string.literal]

# What you see is what you get.
winpath  = 'C:\Users\nodejs\templates'
winpath2 = '\\ServerX\admin$\system32\'
quoted   = 'Tom "Dubs" Preston-Werner'
regex    = '<\i\c*\s*>'


[string.literal.multiline]

regex2 = '''I [dw]on't need \d{2} apples'''
lines  = '''
The first newline is
trimmed in raw strings.
   All other whitespace
   is preserved.
'''


################################################################################
## Integer

# Integers are whole numbers. Positive numbers may be prefixed with a plus sign.
# Negative numbers are prefixed with a minus sign.

[integer]

key1 = +99
key2 = 42
key3 = 0
key4 = -17

[integer.underscores]

# For large numbers, you may use underscores to enhance readability. Each
# underscore must be surrounded by at least one digit.
key1 = 1_000
key2 = 5_349_221
key3 = 1_2_3_4_5     # valid but inadvisable


################################################################################
## Float

# A float consists of an integer part (which may be prefixed with a plus or
# minus sign) followed by a fractional part and/or an exponent part.

[float.fractional]

key1 = +1.0
key2 = 3.1415
key3 = -0.01

[float.exponent]

key1 = 5e+22
key2 = 1e6
key3 = -2E-2

[float.both]

key = 6.626e-34

[float.underscores]

key1 = 9_224_617.445_991_228_313
key2 = 1e1_00


################################################################################
## Boolean

# Booleans are just the tokens you're used to. Always lowercase.

[boolean]

True = true
False = false


################################################################################
## Datetime

# Datetimes are RFC 3339 dates.

[datetime]

key1 = 1979-05-27T07:32:00Z
key2 = 1979-05-27T00:32:00-07:00
key3 = 1979-05-27T00:32:00.999999-07:00


################################################################################
## Array

# Arrays are square brackets with other primitives inside. Whitespace is
# ignored. Elements are separated by commas. Data types may not be mixed.

[array]

key1 = [ 1, 2, 3 ]
key2 = [ "red", "yellow", "green" ]
key3 = [ [ 1, 2 ], [3, 4, 5] ]
key4 = [ [ 1, 2 ], ["a", "b", "c"] ] # this is ok

# Arrays can also be multiline. So in addition to ignoring whitespace, arrays
# also ignore newlines between the brackets.  Terminating commas are ok before
# the closing bracket.

key5 = [
  1, 2, 3
]
key6 = [
  1,
  2, # this is ok
]


################################################################################
## Array of Tables

# These can be expressed by using a table name in double brackets. Each table
# with the same double bracketed name will be an element in the array. The
# tables are inserted in the order encountered.

[[products]]

name = "Hammer"
sku = 738594937

[[products]]

[[products]]

name = "Nail"
sku = 284758393
color = "gray"


# You can create nested arrays of tables as well.

[[fruit]]
  name = "apple"

  [fruit.physical]
    color = "red"
    shape = "round"

  [[fruit.variety]]
    name = "red delicious"

  [[fruit.variety]]
    name = "granny smith"

[[fruit]]
  name = "banana"

  [[fruit.variety]]
    name = "plantain"
    )";

    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::DocumentStart },
        // TODO
        { .kind = TomlEvent::Kind::DocumentEnd },
    };

    Validate(ComplexDocument, expected);
}
