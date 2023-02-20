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

    //TomlEvent(Kind k) : kind(k) {}
    //TomlEvent(Kind k, Span<const StringView> p) : kind(k) { for (const StringView& v : p) path.EmplaceBack(v); }
    //TomlEvent(Kind k, Span<const char*> p) : kind(k) { for (const char* v : p) path.EmplaceBack(v); }
    //TomlEvent(Kind k, Span<const StringView> p, bool value) : kind(k), b(value) { for (const StringView& v : p) path.EmplaceBack(v); }
    //TomlEvent(Kind k, Span<const char*> p, bool value) : kind(k), b(value) { for (const char* v : p) path.EmplaceBack(v); }
    //TomlEvent(Kind k, String&& value) : kind(k), s(Move(value)) {}
    //TomlEvent(Kind k, StringView value) : kind(k), s(value) {}
    //TomlEvent(Kind k, const char* value) : kind(k), s(value) {}
    //TomlEvent(Kind k, bool value) : kind(k), b(value) {}
    //TomlEvent(Kind k, int64_t value) : kind(k), i(value) {}
    //TomlEvent(Kind k, uint64_t value) : kind(k), u(value) {}
    //TomlEvent(Kind k, double value) : kind(k), f(value) {}

    bool operator==(const TomlEvent& x) const
    {
        if (kind != x.kind)
            return false;

        switch (kind)
        {
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

class TomlReaderFixture : public TestFixture
{
public:
    void Validate(StringView input, Span<const TomlEvent> events)
    {
        reader.Read(input, handler);
        handler.Validate(events);
    }

    TomlReaderEventHandler handler;
    TomlReader reader;
};

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_empty, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::EndArray, .u = 0 },
    };
    Validate("key = []", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_spaces, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::EndArray, .u = 0 },
    };
    Validate("  \t key  =\t [ \r\n\t\t]\r\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_one_item, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Int,.i = 6 },
        { .kind = TomlEvent::Kind::EndArray, .u = 1},
    };
    Validate("key = [6]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_multiple_items, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Int, .i = 1 },
        { .kind = TomlEvent::Kind::Int, .i = 2 },
        { .kind = TomlEvent::Kind::Int, .i = 3 },
        { .kind = TomlEvent::Kind::EndArray, .u = 3 },
    };
    Validate("key = [1,\t2,\r\n# this is a comment\r\n\t\t3]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_multiple_heterogenous, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
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
    };
    Validate("key = [1, 0x9ffffffe, 'literal', \"basic\", 1.5, true, '''multiline\n\r\n\nLiteral''', \"\"\"multiline\n\r\n\nBasic\"\"\"]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_nested, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
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
    };
    Validate("key = [1, [2,3], '4', [\"5\", \"6\"]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_tables, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::EndArray, .u = 0 },
    };
    Validate("key = [\n#TODO!\n]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_trailing_comma, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        { .kind = TomlEvent::Kind::Key, .p = { "key" } },
        { .kind = TomlEvent::Kind::StartArray },
        { .kind = TomlEvent::Kind::Int, .i = 1 },
        { .kind = TomlEvent::Kind::Int, .i = 2 },
        { .kind = TomlEvent::Kind::EndArray, .u = 2 },
    };
    Validate("key = [1,2,]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        {.kind = TomlEvent::Kind::StartTable, .s = ".test", .b = true },
        {.kind = TomlEvent::Kind::EndTable, .u = 0 },
    };
    Validate("[[test]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_num, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        {.kind = TomlEvent::Kind::StartTable, .s = ".123", .b = true },
        {.kind = TomlEvent::Kind::EndTable, .u = 0 },
    };
    Validate("[[123]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_quoted, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        {.kind = TomlEvent::Kind::StartTable, .s = ".test\n123", .b = true },
        {.kind = TomlEvent::Kind::EndTable, .u = 0 },
    };
    Validate("[[\"test\\n123\"]]", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_reader, array_table_nested, TomlReaderFixture)
{
    const TomlEvent expected[] =
    {
        {.kind = TomlEvent::Kind::StartTable, .s = ".test.123.name with spaces", .b = true },
        {.kind = TomlEvent::Kind::EndTable, .u = 0 },
    };
    Validate("[[test.123.\"name with spaces\"]]", expected);
}
