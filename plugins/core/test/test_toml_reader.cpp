// Copyright Chad Engler

#include "he/core/toml_reader.h"

#include "he/core/enum_ops.h"
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

    void ValidateString(StringView inputStr, StringView expectedStr)
    {
        const TomlEvent expected[] =
        {
            { .kind = TomlEvent::Kind::String, .s = expectedStr },
        };

        String doc = "key = ";
        doc += inputStr;
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
        { .kind = TomlEvent::Kind::EndArray, .u = 0 },
        { .kind = TomlEvent::Kind::DocumentEnd },
    };
    Validate("key = [\n#TODO!\n]", expected);
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
    Validate("key = \"test\ntest\"", TomlReadError::InvalidValue);
    Validate("key = \"test\0test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\btest\"", TomlReadError::InvalidValue);

    // Invalid escape sequences
    Validate("key = \"test\\mtest\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\xtest\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\ltest\"", TomlReadError::InvalidValue);

    // Invalid unicode sequences
    Validate("key = \"test\\ud800test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\ud900test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\udffftest\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\U0000d800test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\U0000d900test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\U0000dffftest\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\uD800test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\uD900test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\uDfFFtest\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\U0000D800test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\U0000D900test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\U0000DFFFtest\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\U00110000test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\Uaa110000test\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\u000g\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\u00GG\"", TomlReadError::InvalidValue);
    Validate("key = \"test\\uzzzz\"", TomlReadError::InvalidValue);
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
HE_TEST_F(core, toml_reader, complex, TomlReaderFixture)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, empty, TomlReaderFixture)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, float, TomlReaderFixture)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, inline_table, TomlReaderFixture)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, integer, TomlReaderFixture)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, key, TomlReaderFixture)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, literal_string, TomlReaderFixture)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, table, TomlReaderFixture)
{
}
