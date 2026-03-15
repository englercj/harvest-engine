// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/kdl_reader.h"
#include "he/core/kdl_document.h"
#include "he/core/kdl_document_fmt.h"

#include "he/core/math.h"
#include "he/core/test.h"
#include "he/core/types.h"
#include "he/core/optional_fmt.h"
#include "he/core/variant_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
struct KdlEvent
{
    enum class Kind
    {
        StartDocument,
        EndDocument,
        Version,
        Comment,
        StartComment,
        EndComment,
        StartNode,
        EndNode,
        Argument,
        Property,
    };

    Kind kind{ Kind::StartComment };
    String name{};
    Optional<String> type{};
    KdlValue value{};

    [[nodiscard]] bool operator==(const KdlEvent& x) const
    {
        if (kind != x.kind || name != x.name || type != x.type)
            return false;

        if (value.IsFloat() && x.value.IsFloat())
        {
            const double a = value.Float();
            const double b = x.value.Float();
            return (IsNan(a) && IsNan(b)) || a == b;
        }

        return value == x.value;
    }
    [[nodiscard]] bool operator!=(const KdlEvent& x) const { return !(*this == x); }
};

template <>
const char* he::EnumTraits<KdlEvent::Kind>::ToString(KdlEvent::Kind x) noexcept
{
    switch (x)
    {
        case KdlEvent::Kind::StartDocument: return "StartDocument";
        case KdlEvent::Kind::EndDocument: return "EndDocument";
        case KdlEvent::Kind::Version: return "Version";
        case KdlEvent::Kind::Comment: return "Comment";
        case KdlEvent::Kind::StartComment: return "StartComment";
        case KdlEvent::Kind::EndComment: return "EndComment";
        case KdlEvent::Kind::StartNode: return "StartNode";
        case KdlEvent::Kind::EndNode: return "EndNode";
        case KdlEvent::Kind::Argument: return "Argument";
        case KdlEvent::Kind::Property: return "Property";
    }

    return "<unknown>";
}

template <>
struct he::Formatter<KdlEvent>
{
    using Type = KdlEvent;

    constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

    void Format(String& out, const KdlEvent& evt) const
    {
        FormatTo(out, "{{ {}", evt.kind);

        switch (evt.kind)
        {
            case KdlEvent::Kind::StartDocument: break;
            case KdlEvent::Kind::EndDocument: break;
            case KdlEvent::Kind::Version: FormatTo(out, ", value = {}", evt.value); break;
            case KdlEvent::Kind::Comment: FormatTo(out, ", value = {}", evt.value); break;
            case KdlEvent::Kind::StartComment: break;
            case KdlEvent::Kind::EndComment: break;
            case KdlEvent::Kind::StartNode: FormatTo(out, ", name = {}, type = {}", evt.name, evt.type); break;
            case KdlEvent::Kind::EndNode: break;
            case KdlEvent::Kind::Argument: FormatTo(out, ", value = {}, type = {}", evt.value, evt.type); break;
            case KdlEvent::Kind::Property: FormatTo(out, ", name = {}, value = {}, type = {}", evt.name, evt.value, evt.type); break;
        }

        out += " }";
    }
};

// ------------------------------------------------------------------------------------------------
class KdlReaderEventHandler : public KdlReader::Handler
{
public:
    void Validate(StringView input, Span<const KdlEvent> expected) const
    {
        HE_EXPECT_EQ(m_events.Size(), expected.Size());

        for (uint32_t i = 0; i < expected.Size(); ++i)
        {
            HE_EXPECT_EQ(m_events[i], expected[i], i, input);
        }
    }

    void Reset()
    {
        m_events.Clear();
    }

private:
    bool StartDocument() override
    {
        m_events.PushBack({ .kind = KdlEvent::Kind::StartDocument });
        return true;
    }

    bool EndDocument() override
    {
        m_events.PushBack({ .kind = KdlEvent::Kind::EndDocument });
        return true;
    }

    bool Version(StringView value) override
    {
        m_events.PushBack({ .kind = KdlEvent::Kind::Version, .value = value });
        return true;
    }

    bool Comment(StringView value) override
    {
        m_events.PushBack({ .kind = KdlEvent::Kind::Comment, .value = value });
        return true;
    }

    bool StartComment() override
    {
        m_events.PushBack({ .kind = KdlEvent::Kind::StartComment });
        return true;
    }

    bool EndComment() override
    {
        m_events.PushBack({ .kind = KdlEvent::Kind::EndComment });
        return true;
    }

    bool StartNode(StringView name, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::StartNode, .name = name, .type = *type });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::StartNode, .name = name });
        return true;
    }

    bool EndNode() override
    {
        m_events.PushBack({ .kind = KdlEvent::Kind::EndNode });
        return true;
    }

    bool Argument(bool value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .value = value });
        return true;
    }

    bool Argument(int64_t value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .value = value });
        return true;
    }

    bool Argument(uint64_t value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .value = value });
        return true;
    }

    bool Argument(double value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .value = value });
        return true;
    }

    bool Argument(StringView value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .value = value });
        return true;
    }

    bool Argument(nullptr_t, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .type = *type, .value = nullptr });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Argument, .value = nullptr });
        return true;
    }

    bool Property(StringView name, bool value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .value = value });
        return true;
    }

    bool Property(StringView name, int64_t value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .value = value });
        return true;
    }

    bool Property(StringView name, uint64_t value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .value = value });
        return true;
    }

    bool Property(StringView name, double value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .value = value });
        return true;
    }

    bool Property(StringView name, StringView value, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .type = *type, .value = value });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .value = value });
        return true;
    }

    bool Property(StringView name, nullptr_t, const StringView* type) override
    {
        if (type)
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .type = *type, .value = nullptr });
        else
            m_events.PushBack({ .kind = KdlEvent::Kind::Property, .name = name, .value = nullptr });
        return true;
    }

private:
    Vector<KdlEvent> m_events;
};

// ------------------------------------------------------------------------------------------------
class KdlReaderFixture : public TestFixture
{
public:
    void Validate(StringView input, Span<const KdlEvent> events)
    {
        handler.Reset();
        const KdlReadResult result = reader.Read(input, handler);
        HE_EXPECT(result, result.error, result.line, result.column, result.expected, input);
        handler.Validate(input, events);
    }

    void Validate(StringView input, KdlReadError expected)
    {
        handler.Reset();
        const KdlReadResult result = reader.Read(input, handler);
        HE_EXPECT(!result, result.error, expected, input);
        HE_EXPECT_EQ(result.error, expected, input);
    }

    void ValidateNode(StringView input, StringView expectedValue)
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartNode, .name = expectedValue },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndDocument },
        };

        Validate(input, expected);
    }

    template <typename T>
    void ValidateValue(StringView input, T expectedValue)
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartNode, .name = "node" },
            { .kind = KdlEvent::Kind::Argument, .value = expectedValue },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndDocument },
        };

        String doc = "node ";
        doc += input;
        Validate(doc, expected);
    }

    KdlReaderEventHandler handler;
    KdlReader reader;
};

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, version, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::Version, .value = "2" },
        { .kind = KdlEvent::Kind::EndDocument },
    };
    Validate("/-kdl-version 2\n", expected);
    Validate("/- kdl-version 2\n", expected);
    Validate("/-          kdl-version            2\n", expected);

    Validate("/- kdl-version 1", KdlReadError::InvalidVersion);
    Validate("/- kdl-version 2", KdlReadError::UnexpectedEof);
    Validate("/- kdl-version 21", KdlReadError::InvalidToken);
    Validate("/- kdl-version 2s", KdlReadError::InvalidToken);
    Validate("/- kdl-version 2!!!", KdlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, empty, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::EndDocument },
    };
    Validate("", expected);
    Validate("\n\r\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, comments_single_line, KdlReaderFixture)
{
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::Comment, .value = "first comment" },
            { .kind = KdlEvent::Kind::Comment, .value = "second comment" },
            { .kind = KdlEvent::Kind::Comment, .value = "last one" },
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("//first comment\n\n    \t// second comment\r\n\r\n //last one", expected);
    }
    
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartNode, .name = "my-node" },
            { .kind = KdlEvent::Kind::Argument, .value = 1u },
            { .kind = KdlEvent::Kind::Argument, .value = 2u },
            { .kind = KdlEvent::Kind::Comment, .value = "comments are ok after \\"},
            { .kind = KdlEvent::Kind::Argument, .value = 3u },
            { .kind = KdlEvent::Kind::Argument, .value = 4u },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::Comment, .value = "This is the actual end of the Node."},
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("my-node 1 2 \\  // comments are ok after \\\n        3 4    // This is the actual end of the Node.", expected);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, comments_multi_line, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::Comment, .value = "first comment\n" },
        { .kind = KdlEvent::Kind::Comment, .value = "second comment\nhas multiple\n\nlines" },
        { .kind = KdlEvent::Kind::EndDocument },
    };
    Validate("/*first comment\n*/\n    \t/*   second comment\r\nhas multiple\n\r\nlines*/", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, comments_slashdash, KdlReaderFixture)
{
    // Comment out an entire node
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::StartNode, .name = "node" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("/- node", expected);
    }

    // Comment out an argument
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartNode, .name = "node" },
            { .kind = KdlEvent::Kind::Argument, .value = true },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::Argument, .value = "arg" },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::Argument, .value = "last" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("node #true /-arg last", expected);
    }

    // Comment out a property
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartNode, .name = "node" },
            { .kind = KdlEvent::Kind::Property, .name = "prop1", .value = true },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::Property, .name = "prop2", .value = "arg" },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::Property, .name = "prop3", .value = "last" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("node prop1=#true /-prop2=arg prop3=last", expected);
    }

    // Comment out a child block
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartNode, .name = "node" },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::StartNode, .name = "child" },
            { .kind = KdlEvent::Kind::StartNode, .name = "child2" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::StartNode, .name = "child3" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("node /-{\n    child {\n        child2\n    }\n    child3\n}", expected);
    }

    // Nested slashdashes
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartNode, .name = "node" },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::StartNode, .name = "child" },
            { .kind = KdlEvent::Kind::StartNode, .name = "child2" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::StartNode, .name = "child3" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("node /-{\n    /-child {\n        child2\n    }\n    child3\n}", expected);
    }

    // Nested slashdashes with newline and comment separators
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartNode, .name = "node" },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::Comment, .value = "this child node is commented out" },
            { .kind = KdlEvent::Kind::StartNode, .name = "child" },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::Comment, .value = "this one too" },
            { .kind = KdlEvent::Kind::StartNode, .name = "child2" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::StartNode, .name = "child3" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("node /-\t \t{\n    /-\n\n//this child node is commented out  \r\nchild {\n        /- /* this one too */ child2\n    }\n    child3\n}", expected);
    }

    // A node with the kdl-version marker still works as a normal node
    {
        const KdlEvent expected[] =
        {
            { .kind = KdlEvent::Kind::StartDocument },
            { .kind = KdlEvent::Kind::StartComment },
            { .kind = KdlEvent::Kind::StartNode, .name = "kdl-version" },
            { .kind = KdlEvent::Kind::EndNode },
            { .kind = KdlEvent::Kind::EndComment },
            { .kind = KdlEvent::Kind::EndDocument },
        };
        Validate("/-kdl-version", expected);
        Validate("/- kdl-version", expected);
        Validate("/-     kdl-version a1", KdlReadError::InvalidVersion);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, node_empty, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::StartNode, .name = "node" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndDocument },
    };
    Validate("node", expected);
    Validate("\nnode\r\n", expected);
    Validate("node;", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, node_multiple, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        {.kind = KdlEvent::Kind::StartDocument },
        {.kind = KdlEvent::Kind::StartNode, .name = "node" },
        {.kind = KdlEvent::Kind::EndNode },
        {.kind = KdlEvent::Kind::StartNode, .name = "node" },
        {.kind = KdlEvent::Kind::EndNode },
        {.kind = KdlEvent::Kind::StartNode, .name = "node" },
        {.kind = KdlEvent::Kind::EndNode },
        {.kind = KdlEvent::Kind::EndDocument },
    };
    Validate("node\nnode\nnode", expected);
    Validate("\nnode\r\nnode\r\nnode", expected);
    Validate("node;node;node;", expected);
    Validate("node;  node;;   node;;;;", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, node_args, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::StartNode, .name = "node" },
        { .kind = KdlEvent::Kind::Argument, .value = true },
        { .kind = KdlEvent::Kind::Argument, .type = "i32", .value = -123ll },
        { .kind = KdlEvent::Kind::Argument, .type = "u32", .value = 123ull },
        { .kind = KdlEvent::Kind::Argument, .value = 5.0 },
        { .kind = KdlEvent::Kind::Argument, .value = "arg" },
        { .kind = KdlEvent::Kind::Argument, .value = "ello" },
        { .kind = KdlEvent::Kind::Argument, .value = nullptr },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndDocument },
    };
    Validate("node #true (i32)-123 (u32)123 5.0 \"arg\" #\"ello\"# #null", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, node_props, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::StartNode, .name = "node" },
        { .kind = KdlEvent::Kind::Property, .name = "bool", .value = true },
        { .kind = KdlEvent::Kind::Property, .name = "int", .type = "i32", .value = -123ll },
        { .kind = KdlEvent::Kind::Property, .name = "uint", .type = "u32", .value = 123ull },
        { .kind = KdlEvent::Kind::Property, .name = "float", .value = 5.0 },
        { .kind = KdlEvent::Kind::Property, .name = "str", .value = "arg" },
        { .kind = KdlEvent::Kind::Property, .name = "str2", .value = "ello" },
        { .kind = KdlEvent::Kind::Property, .name = "nullptr", .value = nullptr },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndDocument },
    };
    Validate("node bool=#true int=(i32)-123 uint=(u32)123 float=5.0 str=arg str2=#\"ello\"# nullptr=#null", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, node_children_empty, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::StartNode, .name = "node" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndDocument },
    };
    Validate("node {\n}\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, node_children, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::StartNode, .name = "node" },
        { .kind = KdlEvent::Kind::StartNode, .name = "child" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndDocument },
    };
    Validate("node {\n    child\n}\n", expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, value_boolean, KdlReaderFixture)
{
    ValidateValue("#true", true);
    ValidateValue("#false", false);

    ValidateValue("True", "True");
    ValidateValue("TRUE", "TRUE");
    ValidateValue("False", "False");
    ValidateValue("FALSE", "FALSE");

    Validate("node true", KdlReadError::InvalidIdentifier);
    Validate("node false", KdlReadError::InvalidIdentifier);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, value_int, KdlReaderFixture)
{
    // Decimal
    ValidateValue("-0", -0);
    ValidateValue("-1", -1);
    ValidateValue("-456789", -456789);
    ValidateValue("-9223372036854775808", Limits<int64_t>::Min);
    ValidateValue("-1_000", -1000);
    ValidateValue("-5_349_221", -5349221);
    ValidateValue("-53_49_221", -5349221);
    ValidateValue("-1_2_3_4_5", -12345);

    // Hex
    ValidateValue("-0x0", 0);
    ValidateValue("-0xdeadbeef", -0xdeadbeefll);
    ValidateValue("-0xdead_beef", -0xdeadbeefll);
    ValidateValue("-0x80000000", Limits<int32_t>::Min);
    ValidateValue("-0x8000000000000000", Limits<int64_t>::Min);

    // Octal
    ValidateValue("-0o0", -0);
    ValidateValue("-0o755", -0755);
    ValidateValue("-0o655", -0655);

    // Bin
    ValidateValue("-0b0", -0);
    ValidateValue("-0b1", -0b1);
    ValidateValue("-0b11", -0b11);
    ValidateValue("-0b111", -0b111);
    ValidateValue("-0b1111", -0b1111);
    ValidateValue("-0b1010101", -0b1010101);
    ValidateValue("-0b0101_0101", -0b01010101);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, value_uint, KdlReaderFixture)
{
    // Decimal
    ValidateValue("0", 0u);
    ValidateValue("+0", 0u);
    ValidateValue("1", 1u);
    ValidateValue("+1", 1u);
    ValidateValue("456789", 456789u);
    ValidateValue("+456789", 456789u);
    ValidateValue("9223372036854775807", static_cast<uint64_t>(Limits<int64_t>::Max));
    ValidateValue("18446744073709551615", Limits<uint64_t>::Max);
    ValidateValue("1_000", 1000u);
    ValidateValue("5_349_221", 5349221u);
    ValidateValue("53_49_221", 5349221u);
    ValidateValue("1_2_3_4_5", 12345u);

    // Hex
    ValidateValue("0x0", 0u);
    ValidateValue("0xdeadbeef", 0xdeadbeefu);
    ValidateValue("0xdead_beef", 0xdeadbeefu);
    ValidateValue("0xffffffff", Limits<uint32_t>::Max);
    ValidateValue("0xffffffffffffffff", Limits<uint64_t>::Max);

    // Octal
    ValidateValue("0o0", 0u);
    ValidateValue("0o755", 0755u);
    ValidateValue("0o655", 0655u);

    // Bin
    ValidateValue("0b0", 0u);
    ValidateValue("0b1", 0b1u);
    ValidateValue("0b11", 0b11u);
    ValidateValue("0b111", 0b111u);
    ValidateValue("0b1111", 0b1111u);
    ValidateValue("0b1010101", 0b1010101u);
    ValidateValue("0b0101_0101", 0b01010101u);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, value_float, KdlReaderFixture)
{
    // Zeroes
    ValidateValue("0.0", 0.0);
    ValidateValue("+0.0", 0.0);
    ValidateValue("-0.0", 0.0);
    ValidateValue("0e0", 0.0);
    ValidateValue("-0e0", 0.0);
    ValidateValue("0e-0", 0.0);
    ValidateValue("0e+0", 0.0);
    ValidateValue("-0e-0", 0.0);
    ValidateValue("-0e+0", 0.0);
    ValidateValue("+0e-0", 0.0);
    ValidateValue("+0e+0", 0.0);
    ValidateValue("0e10", 0.0);
    ValidateValue("0e-10", 0.0);
    ValidateValue("0.0e0", 0.0);
    ValidateValue("0.000e+0", 0.0);
    ValidateValue("0.00e-0", 0.0);
    ValidateValue("-0.00e+0", 0.0);
    ValidateValue("+0.00e-100", 0.0);

    // Fractional
    ValidateValue("+14.1345", 14.1345);
    ValidateValue("-14.0001345", -14.0001345);
    ValidateValue("0.0001345", 0.0001345);
    ValidateValue("10.0", 10.0);

    // Exponential
    ValidateValue("2e3", 2e3);
    ValidateValue("2e-3", 2e-3);
    ValidateValue("2e0", 2e0);
    ValidateValue("2e+0", 2e0);
    ValidateValue("2e-0", 2e0);
    ValidateValue("+2e-1", 2e-1);
    ValidateValue("-2e-1", -2e-1);
    ValidateValue("1e+123", 1e123);
    ValidateValue("1e-123", 1e-123);

    ValidateValue("2E3", 2e3);
    ValidateValue("2E-3", 2e-3);
    ValidateValue("2E0", 2e0);
    ValidateValue("2E+0", 2e0);
    ValidateValue("2E-0", 2e0);
    ValidateValue("+2E-1", 2e-1);
    ValidateValue("-2E-1", -2e-1);
    ValidateValue("1E+123", 1e123);
    ValidateValue("1E-123", 1e-123);

    // FractionalExponential
    ValidateValue("2.0e3", 2e3);
    ValidateValue("2.1e-3", 2.1e-3);
    ValidateValue("2.54e0", 2.54e0);
    ValidateValue("2.000e+0", 2.0);
    ValidateValue("2.09e-0", 2.09);
    ValidateValue("+2.2e-1", 0.22);
    ValidateValue("-2.12e-1", -0.212);
    ValidateValue("1.01e+123", 101e121);
    ValidateValue("1.01e-123", 101e-125);
    ValidateValue("2.0E3", 2e3);
    ValidateValue("2.1E-3", 0.0021);
    ValidateValue("2.54E0", 2.54);
    ValidateValue("2.000E+0", 2.0);
    ValidateValue("2.09E-0", 2.09);
    ValidateValue("+2.2E-1", 0.22);
    ValidateValue("-2.12E-1", -0.212);
    ValidateValue("1.01E+123", 101e121);
    ValidateValue("1.01E-123", 101e-125);

    // Leading zeros are allowed
    ValidateValue("01.0", 1.0);
    ValidateValue("00.0", 0.0);
    ValidateValue("02e1", 2e1);
    ValidateValue("-02e1", -2e1);
    ValidateValue("+02e-1", 2e-1);
    ValidateValue("+02.01e-1", 2.01e-1);

    // Special float values
    ValidateValue("#inf", Limits<double>::Infinity);
    ValidateValue("#-inf", -Limits<double>::Infinity);
    ValidateValue("#nan", Limits<double>::NaN);

    // Overflow
    Validate("node 1.0e1000", KdlReadError::InvalidNumber);
    Validate("node -1.0e1000", KdlReadError::InvalidNumber);

    // Fractional part cannot be empty
    Validate("node 1.", KdlReadError::InvalidNumber);
    Validate("node 1.e10", KdlReadError::InvalidNumber);
    Validate("node -1.", KdlReadError::InvalidNumber);
    Validate("node 0.", KdlReadError::InvalidNumber);
    Validate("node +0.", KdlReadError::InvalidNumber);

    // Exponent cannot be empty
    Validate("node 1e", KdlReadError::InvalidNumber);
    Validate("node 1e+", KdlReadError::InvalidNumber);
    Validate("node 1e-", KdlReadError::InvalidNumber);

    // Multiple dots are not allowed
    Validate("node 1..0", KdlReadError::InvalidNumber);
    Validate("node 1.0.0", KdlReadError::InvalidNumber);

    // Multiple exponents are not allowed
    Validate("node 1ee0", KdlReadError::InvalidNumber);
    Validate("node 1e0e0", KdlReadError::InvalidNumber);

    // Only inf can have a sign
    Validate("node #+inf", KdlReadError::InvalidToken);
    Validate("node #+nan", KdlReadError::InvalidToken);
    Validate("node #-nan", KdlReadError::InvalidToken);

    // Inf and NaN must be lowercase
    Validate("node #Inf", KdlReadError::InvalidToken);
    Validate("node #-Inf", KdlReadError::InvalidToken);
    Validate("node #INF", KdlReadError::InvalidToken);
    Validate("node #-INF", KdlReadError::InvalidToken);
    Validate("node #NAN", KdlReadError::InvalidToken);
    Validate("node #NaN", KdlReadError::InvalidToken);
    Validate("node #Nan", KdlReadError::InvalidToken);
    Validate("node #naninf", KdlReadError::InvalidToken);
    Validate("node #infnan", KdlReadError::InvalidToken);
    Validate("node #infity", KdlReadError::InvalidToken);
    Validate("node #notanum", KdlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, value_string_identifier, KdlReaderFixture)
{
    // Basic identifiers
    ValidateValue("test", "test");
    ValidateValue("é", "é");
    ValidateValue("bare_key", "bare_key");
    ValidateValue("bare-key", "bare-key");
    ValidateValue("Fuß", "Fuß");
    ValidateValue("😂", "😂");
    ValidateValue("key_-23_-", "key_-23_-");
    ValidateValue("-.stuff", "-.stuff");
    ValidateValue(".stuff", ".stuff");
    ValidateValue("-stuff", "-stuff");
    ValidateValue("Fuß", "Fuß");
    ValidateValue("😂", "😂");
    ValidateValue("汉语大字典", "汉语大字典");
    ValidateValue("辭源", "辭源");
    ValidateValue("பெண்டிரேம்", "பெண்டிரேம்");

    // Cannot start with a digit
    Validate("1test", KdlReadError::InvalidToken);
    Validate("1.0", KdlReadError::InvalidToken);

    // No non-id characters are allowed
    Validate("node te(st", KdlReadError::InvalidToken);
    Validate("node te)st", KdlReadError::InvalidToken);
    Validate("node te{st", KdlReadError::InvalidToken);
    Validate("node te}st", KdlReadError::InvalidToken);
    Validate("node te[st", KdlReadError::InvalidToken);
    Validate("node te]st", KdlReadError::InvalidToken);
    Validate("node te/st", KdlReadError::InvalidToken);
    Validate("node te\\st", KdlReadError::InvalidToken);
    Validate("node te\"st", KdlReadError::InvalidToken);
    Validate("node te#st", KdlReadError::InvalidToken);
    Validate("node te\x7fst", KdlReadError::DisallowedUtf8);

    // No number-like sequences are allowed
    Validate("node 1.0v2", KdlReadError::InvalidToken);
    Validate("node -1em", KdlReadError::InvalidNumber);
    Validate("node .1", KdlReadError::InvalidIdentifier);
    Validate("node .0", KdlReadError::InvalidIdentifier);
    Validate("node -.1", KdlReadError::InvalidIdentifier);
    Validate("node +.0", KdlReadError::InvalidIdentifier);
    Validate("node .25.1", KdlReadError::InvalidIdentifier);

    // No keywords are allowed
    Validate("node inf", KdlReadError::InvalidIdentifier);
    //Validate("node +inf", KdlReadError::InvalidIdentifier);
    Validate("node -inf", KdlReadError::InvalidIdentifier);
    Validate("node nan", KdlReadError::InvalidIdentifier);
    //Validate("node +nan", KdlReadError::InvalidIdentifier);
    //Validate("node -nan", KdlReadError::InvalidIdentifier);
    Validate("node true", KdlReadError::InvalidIdentifier);
    Validate("node false", KdlReadError::InvalidIdentifier);
    Validate("node null", KdlReadError::InvalidIdentifier);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, value_string_quoted, KdlReaderFixture)
{
    // Basic encodings
    ValidateValue("\"\"", "");
    ValidateValue("\"test\"", "test");
    ValidateValue("\"test\\b\\t\\n\\f\\r\\\"\\\\\"", "test\b\t\n\f\r\"\\");
    ValidateValue("\"\\u{0001}\"", "\x01");
    ValidateValue("\"\\u{000001}\"", "\x01");
    ValidateValue("\"\\u{0000e9}\"", "é");
    ValidateValue("\"test\\u{d7ff}test\"", "test\xed\x9f\xbftest");
    ValidateValue("\"test\\u{d7FF}test\"", "test\xed\x9f\xbftest");
    ValidateValue("\"test\\u{e000}test\"", "test\xee\x80\x80test");
    ValidateValue("\"test\\u{E000}test\"", "test\xee\x80\x80test");
    ValidateValue("\"test\\u{00e000}test\"", "test\xee\x80\x80test");
    ValidateValue("\"test\\u{00E000}test\"", "test\xee\x80\x80test");
    ValidateValue("\"test\\u{10ffff}test\"", "test\xf4\x8f\xbf\xbftest");
    ValidateValue("\"test\\u{10FFFF}test\"", "test\xf4\x8f\xbf\xbftest");
    ValidateValue("\"Fuß\"", "Fuß");
    ValidateValue("\"😂\"", "😂");
    ValidateValue("\"汉语大字典\"", "汉语大字典");
    ValidateValue("\"辭源\"", "辭源");
    ValidateValue("\"பெண்டிரேம்\"", "பெண்டிரேம்");
    ValidateValue("\"╠═╣\"", "╠═╣");
    ValidateValue("\"⋰∫∬∭⋱\"", "⋰∫∬∭⋱");
    ValidateValue("\"C:\\\\Program Files (x86)\\\\Microsoft\\\\Edge\\\\Application\\\\msedge.exe\"", "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");

    // Multiline strings
    ValidateValue("\"\"\"\n\"\"\"", "");
    ValidateValue("\"\"\"\ntest\n\"\"\"", "test");
    ValidateValue("\"\"\"\ntest\ntest\n\"\"\"", "test\ntest");
    ValidateValue("\"\"\"\r\n   test\ntest\r\n\"\"\"", "   test\ntest");
    ValidateValue("\"\"\"\n   test\r\ntest\n\"\"\"", "   test\ntest");

    // Multiline strings with indentation
    ValidateValue("\"\"\"\ntest\ntest\n\"\"\"", "test\ntest");
    ValidateValue("\"\"\"\n  test\n  test\n  \"\"\"", "test\ntest");
    ValidateValue("\"\"\"\n  test\n    test\n  \"\"\"", "test\n  test");
    ValidateValue("\"\"\"\n\t\ttest\n\t\ttest\r\n\t\t\"\"\"", "test\ntest");
    ValidateValue("\"\"\"\n\u00a0test\n\u00a0\u00a0test\n\u00a0\"\"\"", "test\n\u00a0test");
    ValidateValue("\"\"\"\n \u00a0 \ttest\n \u00a0 \ttest\n \u00a0 \t\"\"\"", "test\ntest");
    ValidateValue("\"\"\"\n\u2002\u2003test\n\u2002\u2003test\n\u2002\u2003\"\"\"", "test\ntest");

    // Escape whitespace
    ValidateValue("\"\"\"\ntest  \\\n    \r\n   \n   xxx\nyyy\r\nzzz\n\"\"\"", "test  xxx\nyyy\nzzz");
    ValidateValue("\"\"\"\ntest  \\\n    \r\n   \n   xxx\nyyy\r\nzzz\n\"\"\"", "test  xxx\nyyy\nzzz");
    ValidateValue("\"\"\"\ntest  \\\r\n    \r\n   \n   xxx\nyyy\r\nzzz\n\"\"\"", "test  xxx\nyyy\nzzz");

    // Escape sequences
    ValidateValue("\"test\\b\\t\\n\\f\\r\\\"\\\\\"", "test\b\t\n\f\r\"\\");
    ValidateValue("\"\\u{0001}\"", "\x01");
    ValidateValue("\"\\u{000001}\"", "\x01");
    ValidateValue("\"\\u{0000e9}\"", "é");
    ValidateValue("\"test\\u{d7ff}test\"", "test\xed\x9f\xbftest");
    ValidateValue("\"test\\u{d7FF}test\"", "test\xed\x9f\xbftest");
    ValidateValue("\"test\\u{e000}test\"", "test\xee\x80\x80test");
    ValidateValue("\"test\\u{E000}test\"", "test\xee\x80\x80test");
    ValidateValue("\"test\\u{00e000}test\"", "test\xee\x80\x80test");
    ValidateValue("\"test\\u{00E000}test\"", "test\xee\x80\x80test");
    ValidateValue("\"test\\u{10ffff}test\"", "test\xf4\x8f\xbf\xbftest");
    ValidateValue("\"test\\u{10FFFF}test\"", "test\xf4\x8f\xbf\xbftest");

    // Unescaped control characters
    Validate("node \"test\ntest\"", KdlReadError::InvalidControlChar);
    Validate("node \"test\0test\"", KdlReadError::UnexpectedEof);
    Validate("node \"test\btest\"", KdlReadError::DisallowedUtf8);

    // Invalid escape sequences
    Validate("node \"test\\mtest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\ltest\"", KdlReadError::InvalidEscapeSequence);

    // Indent of each line must match the last line exactly
    Validate("node \"\"\"\n  test\n  test\r\n    \"\"\"", KdlReadError::InvalidToken);
    Validate("node \"\"\"\n\ttest\n\ttest\r\n\t\t\"\"\"", KdlReadError::InvalidToken);
    Validate("node \"\"\"\n \ttest\n \ttest\r\n    \"\"\"", KdlReadError::InvalidToken);
    Validate("node \"\"\"\n\u2002test\n\u2002test\n\u2002\u2003\"\"\"", KdlReadError::InvalidToken);

    // Invalid escape sequence
    Validate("node \"test\\mtest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\ltest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\xtest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\Utest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\U00test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\Ua0test\"", KdlReadError::InvalidEscapeSequence);

    // Invalid unicode sequences
    Validate("node \"test\\ud800test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\ud900test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\udffftest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u0000d800test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u0000d900test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u0000dffftest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\uD800test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\uD900test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\uDfFFtest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u0000D800test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u0000D900test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u0000DFFFtest\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u00110000test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\uaa110000test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u000g\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u00GG\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\uzzzz\"", KdlReadError::InvalidEscapeSequence);

    Validate("node \"test\\u{d800}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{d900}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{dfff}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{0000d800}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{0000d900}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{0000dfff}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{D800}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{D900}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{DfFF}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{00D800}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{00D900}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{00DFFF}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{110000}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"\\u{00000001}\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"\\u{000000e9}\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{aa110000}test\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{000g}\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{00GG}\"", KdlReadError::InvalidEscapeSequence);
    Validate("node \"test\\u{zzzz}\"", KdlReadError::InvalidEscapeSequence);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, value_string_raw, KdlReaderFixture)
{
    // Basic encodings
    ValidateValue("#\"  \t test \\test\"test\"#", "  \t test \\test\"test");

    ValidateValue("#\"\"#", "");
    ValidateValue("#\"test\"#", "test");
    ValidateValue("#\"test\\b\\t\\n\\f\\r\\\"\\\\\"#", "test\\b\\t\\n\\f\\r\\\"\\\\");
    ValidateValue("#\"\\u0001\"#", "\\u0001");
    ValidateValue("#\"\\U00000001\"#", "\\U00000001");
    ValidateValue("#\"\\U000000e9\"#", "\\U000000e9");
    ValidateValue("#\"test\\ud7fftest\"#", "test\\ud7fftest");
    ValidateValue("#\"test\\ud7FFtest\"#", "test\\ud7FFtest");
    ValidateValue("#\"test\\ue000test\"#", "test\\ue000test");
    ValidateValue("#\"test\\uE000test\"#", "test\\uE000test");
    ValidateValue("#\"test\\u0000e000test\"#", "test\\u0000e000test");
    ValidateValue("#\"test\\u0000E000test\"#", "test\\u0000E000test");
    ValidateValue("#\"test\\u0010fffftest\"#", "test\\u0010fffftest");
    ValidateValue("#\"test\\u0010FFFFtest\"#", "test\\u0010FFFFtest");
    ValidateValue("#\"Fuß\"#", "Fuß");
    ValidateValue("#\"😂\"#", "😂");
    ValidateValue("#\"汉语大字典\"#", "汉语大字典");
    ValidateValue("#\"辭源\"#", "辭源");
    ValidateValue("#\"பெண்டிரேம்\"#", "பெண்டிரேம்");
    ValidateValue("#\"╠═╣\"#", "╠═╣");
    ValidateValue("#\"⋰∫∬∭⋱\"#", "⋰∫∬∭⋱");
    ValidateValue("#\"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe\"#", "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
    ValidateValue("#\"\"That,\" she said, \"is still pointless.\"\"#", "\"That,\" she said, \"is still pointless.\"");

    // Mixing quotes and raw delimiters
    ValidateValue("####\"\"####", "");
    ValidateValue("##\" '\"#' \"##", " '\"#' ");
    ValidateValue("##\" '\"#\"\"' \"##", " '\"#\"\"' ");
    ValidateValue("##\"'\"#\"#\"#'\"##", "'\"#\"#\"#'");

    // Multiline strings
    ValidateValue("#\"\"\"\n\"\"\"#", "");
    ValidateValue("#\"\"\"\ntest\n\"\"\"#", "test");
    ValidateValue("#\"\"\"\ntest\ntest\n\"\"\"#", "test\ntest");
    ValidateValue("#\"\"\"\r\n   test\ntest\r\n\"\"\"#", "   test\ntest");
    ValidateValue("#\"\"\"\n   test\r\ntest\n\"\"\"#", "   test\ntest");

    // Multiline strings with indentation
    ValidateValue("#\"\"\"\ntest\ntest\n\"\"\"#", "test\ntest");
    ValidateValue("#\"\"\"\n  test\n  test\n  \"\"\"#", "test\ntest");
    ValidateValue("#\"\"\"\n  test\n    test\n  \"\"\"#", "test\n  test");
    ValidateValue("#\"\"\"\n\t\ttest\n\t\ttest\r\n\t\t\"\"\"#", "test\ntest");
    ValidateValue("#\"\"\"\n\u00a0test\n\u00a0\u00a0test\n\u00a0\"\"\"#", "test\n\u00a0test");
    ValidateValue("#\"\"\"\n \u00a0 \ttest\n \u00a0 \ttest\n \u00a0 \t\"\"\"#", "test\ntest");
    ValidateValue("#\"\"\"\n\u2002\u2003test\n\u2002\u2003test\n\u2002\u2003\"\"\"#", "test\ntest");

    // Escape whitespace
    ValidateValue("#\"\"\"\ntest  \\\n    \r\n   \n   xxx\nyyy\r\nzzz\n\"\"\"#", "test  \\\n    \n   \n   xxx\nyyy\nzzz");
    ValidateValue("#\"\"\"\ntest  \\\n    \r\n   \n   xxx\nyyy\r\nzzz\n\"\"\"#", "test  \\\n    \n   \n   xxx\nyyy\nzzz");
    ValidateValue("#\"\"\"\ntest  \\\r\n    \r\n   \n   xxx\nyyy\r\nzzz\n\"\"\"#", "test  \\\n    \n   \n   xxx\nyyy\nzzz");

    // Escape sequences
    ValidateValue("#\"test\\b\\t\\n\\f\\r\\\"\\\\\"#", "test\\b\\t\\n\\f\\r\\\"\\\\");
    ValidateValue("#\"\\u0001\"#", "\\u0001");
    ValidateValue("#\"\\U00000001\"#", "\\U00000001");
    ValidateValue("#\"\\U000000e9\"#", "\\U000000e9");
    ValidateValue("#\"test\\ud7fftest\"#", "test\\ud7fftest");
    ValidateValue("#\"test\\ud7FFtest\"#", "test\\ud7FFtest");
    ValidateValue("#\"test\\ue000test\"#", "test\\ue000test");
    ValidateValue("#\"test\\uE000test\"#", "test\\uE000test");
    ValidateValue("#\"test\\u00e000test\"#", "test\\u00e000test");
    ValidateValue("#\"test\\u00E000test\"#", "test\\u00E000test");
    ValidateValue("#\"test\\u10fffftest\"#", "test\\u10fffftest");
    ValidateValue("#\"test\\u10FFFFtest\"#", "test\\u10FFFFtest");

    // Unescaped control characters
    Validate("node #\"test\ntest\"#", KdlReadError::InvalidControlChar);
    Validate("node #\"test\0test\"#", KdlReadError::UnexpectedEof);
    Validate("node #\"test\btest\"#", KdlReadError::DisallowedUtf8);

    // Indent of each line must match the last line exactly
    Validate("node \"\"\"\n  test\n  test\r\n    \"\"\"", KdlReadError::InvalidToken);
    Validate("node \"\"\"\n\ttest\n\ttest\r\n\t\t\"\"\"", KdlReadError::InvalidToken);
    Validate("node \"\"\"\n \ttest\n \ttest\r\n    \"\"\"", KdlReadError::InvalidToken);
    Validate("node \"\"\"\n\u2002test\n\u2002test\n\u2002\u2003\"\"\"", KdlReadError::InvalidToken);
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_reader, document, KdlReaderFixture)
{
    const KdlEvent expected[] =
    {
        { .kind = KdlEvent::Kind::StartDocument },
        { .kind = KdlEvent::Kind::Comment, .value = "Copyright Chad Engler" },
        { .kind = KdlEvent::Kind::StartNode, .name = "boolean" },
        { .kind = KdlEvent::Kind::Property, .name = "bool1", .value = true },
        { .kind = KdlEvent::Kind::Property, .name = "bool2", .value = false },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::Comment, .value = "Integers of various formats" },
        { .kind = KdlEvent::Kind::StartNode, .name = "integers" },
        { .kind = KdlEvent::Kind::StartNode, .name = "decimal" },
        { .kind = KdlEvent::Kind::Property, .name = "int1", .value = 99ull },
        { .kind = KdlEvent::Kind::Property, .name = "int2", .value = 42ull },
        { .kind = KdlEvent::Kind::Property, .name = "int3", .value = 0ull },
        { .kind = KdlEvent::Kind::Property, .name = "int4", .value = -17ll },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "decimal" },
        { .kind = KdlEvent::Kind::Property, .name = "int5", .value = 1000ull },
        { .kind = KdlEvent::Kind::Property, .name = "int6", .value = 5349221ull },
        { .kind = KdlEvent::Kind::Property, .name = "int7", .value = 5349221ull },
        { .kind = KdlEvent::Kind::Property, .name = "int8", .value = 12345ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "hexadecimal" },
        { .kind = KdlEvent::Kind::Property, .name = "hex1", .value = 0xdeadbeefull },
        { .kind = KdlEvent::Kind::Property, .name = "hex2", .value = 0xdeadbeefull },
        { .kind = KdlEvent::Kind::Property, .name = "hex3", .value = 0xdeadbeefull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "octal" },
        { .kind = KdlEvent::Kind::Property, .name = "oct1", .value = 01234567ull },
        { .kind = KdlEvent::Kind::Property, .name = "oct2", .value = 0755ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::Comment, .value = "useful for Unix file permissions" },
        { .kind = KdlEvent::Kind::StartNode, .name = "binary" },
        { .kind = KdlEvent::Kind::Property, .name = "bin1", .value = 0b11010110ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::Comment, .value = "Floats of various formats" },
        { .kind = KdlEvent::Kind::StartNode, .name = "floats" },
        { .kind = KdlEvent::Kind::StartNode, .name = "decimal" },
        { .kind = KdlEvent::Kind::Property, .name = "flt1", .value = 1.0 },
        { .kind = KdlEvent::Kind::Property, .name = "flt2", .value = 3.1415 },
        { .kind = KdlEvent::Kind::Property, .name = "flt3", .value = -0.01 },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "exponent" },
        { .kind = KdlEvent::Kind::Property, .name = "flt4", .value = 5e+22 },
        { .kind = KdlEvent::Kind::Property, .name = "flt5", .value = 1e06 },
        { .kind = KdlEvent::Kind::Property, .name = "flt6", .value = -2e-2 },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "both" },
        { .kind = KdlEvent::Kind::Property, .name = "flt7", .value = 6.626e-34 },
        { .kind = KdlEvent::Kind::Property, .name = "flt8", .value = 224617.445991228 },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "special" },
        { .kind = KdlEvent::Kind::Property, .name = "flt9", .value = Limits<double>::Infinity },
        { .kind = KdlEvent::Kind::Property, .name = "flt10", .value = -Limits<double>::Infinity },
        { .kind = KdlEvent::Kind::Property, .name = "flt11", .value = Limits<double>::NaN },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::Comment, .value = "Strings of various formats" },
        { .kind = KdlEvent::Kind::StartNode, .name = "strings" },
        { .kind = KdlEvent::Kind::StartNode, .name = "escaped" },
        { .kind = KdlEvent::Kind::Argument, .value = "I'm a string. \"You can quote me\". Name\tJosé\nLocation\tSF." },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "multiline" },
        { .kind = KdlEvent::Kind::Argument, .value = "Roses are red\nViolets are blue" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::Comment, .value = "the above multi-line string will be the same as:" },
        { .kind = KdlEvent::Kind::StartNode, .name = "multiline2" },
        { .kind = KdlEvent::Kind::Argument, .value = "Roses are red\nViolets are blue" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::Comment, .value = "The following strings are byte-for-byte equivalent:" },
        { .kind = KdlEvent::Kind::StartNode, .name = "fox" },
        { .kind = KdlEvent::Kind::Argument, .value = "The quick brown fox jumps over the lazy dog." },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "fox2" },
        { .kind = KdlEvent::Kind::Argument, .value = "The quick brown fox jumps over the lazy dog." },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "fox3" },
        { .kind = KdlEvent::Kind::Argument, .value = "The quick brown fox jumps over the lazy dog." },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "quote1" },
        { .kind = KdlEvent::Kind::Argument, .value = R"(Here are two quotation marks: "". Simple enough.)" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "quote2" },
        { .kind = KdlEvent::Kind::Argument, .value = R"(Here are three quotation marks: """.)" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "quote3" },
        { .kind = KdlEvent::Kind::Argument, .value = R"(Here are fifteen quotation marks: """"""""""""""".)" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "winpath" },
        { .kind = KdlEvent::Kind::Argument, .value = R"(C:\Users\nodejs\templates)" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "winpath2" },
        { .kind = KdlEvent::Kind::Argument, .value = R"(\\ServerX\admin$\system32\)" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "quoted" },
        { .kind = KdlEvent::Kind::Argument, .value = R"(Tom "Dubs" Preston-Werner)" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "regex" },
        { .kind = KdlEvent::Kind::Argument, .value = R"(<\i\c*\s*>)" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "regex2" },
        { .kind = KdlEvent::Kind::Argument, .value = R"(I [dw]on't need \d{2} apples)" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "lines" },
        { .kind = KdlEvent::Kind::Argument, .value = "    The first newline is\n    trimmed in multiline strings.\n    All other whitespace\n    is preserved." },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::Comment, .value = "Nested nodes" },
        { .kind = KdlEvent::Kind::StartNode, .name = "tables" },
        { .kind = KdlEvent::Kind::StartNode, .name = "dotted.names.change.nothing" },
        { .kind = KdlEvent::Kind::StartNode, .name = "contributors" },
        { .kind = KdlEvent::Kind::StartNode, .name = "Foo Bar" },
        { .kind = KdlEvent::Kind::Argument, .value = "foo@example.com" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "Baz Qux" },
        { .kind = KdlEvent::Kind::Property, .name = "email", .value = "bazqux@example.com" },
        { .kind = KdlEvent::Kind::Property, .name = "url", .value = "https://example.com/bazqux" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "points" },
        { .kind = KdlEvent::Kind::StartNode, .name = "point" },
        { .kind = KdlEvent::Kind::Property, .name = "x", .value = 1ull },
        { .kind = KdlEvent::Kind::Property, .name = "y", .value = 2ull },
        { .kind = KdlEvent::Kind::Property, .name = "z", .value = 3ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "point" },
        { .kind = KdlEvent::Kind::Property, .name = "x", .value = 7ull },
        { .kind = KdlEvent::Kind::Property, .name = "y", .value = 8ull },
        { .kind = KdlEvent::Kind::Property, .name = "z", .value = 9ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "point" },
        { .kind = KdlEvent::Kind::Property, .name = "x", .value = 2ull },
        { .kind = KdlEvent::Kind::Property, .name = "y", .value = 4ull },
        { .kind = KdlEvent::Kind::Property, .name = "z", .value = 8ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "points" },
        { .kind = KdlEvent::Kind::StartNode, .name = "-" },
        { .kind = KdlEvent::Kind::Argument, .value = 1ull },
        { .kind = KdlEvent::Kind::Argument, .value = 2ull },
        { .kind = KdlEvent::Kind::Argument, .value = 3ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "-" },
        { .kind = KdlEvent::Kind::Argument, .value = 7ull },
        { .kind = KdlEvent::Kind::Argument, .value = 8ull },
        { .kind = KdlEvent::Kind::Argument, .value = 9ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "-" },
        { .kind = KdlEvent::Kind::Argument, .value = 2ull },
        { .kind = KdlEvent::Kind::Argument, .value = 4ull },
        { .kind = KdlEvent::Kind::Argument, .value = 8ull },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::Comment, .value = "Website" },
        { .kind = KdlEvent::Kind::StartNode, .name = "!doctype" },
        { .kind = KdlEvent::Kind::Argument, .value = "html" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "html" },
        { .kind = KdlEvent::Kind::Property, .name = "lang", .value = "en" },
        { .kind = KdlEvent::Kind::StartNode, .name = "head" },
        { .kind = KdlEvent::Kind::StartNode, .name = "meta" },
        { .kind = KdlEvent::Kind::Property, .name = "charset", .value = "utf-8" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "meta" },
        { .kind = KdlEvent::Kind::Property, .name = "name", .value = "viewport" },
        { .kind = KdlEvent::Kind::Property, .name = "content", .value = "width=device-width, initial-scale=1.0" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "meta" },
        { .kind = KdlEvent::Kind::Property, .name = "name", .value = "description" },
        { .kind = KdlEvent::Kind::Property, .name = "content", .value = "kdl is a document language, mostly based on SDLang, with xml-like semantics that looks like you're invoking a bunch of CLI commands!" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "title" },
        { .kind = KdlEvent::Kind::Argument, .value = "kdl - The KDL Document Language" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "link" },
        { .kind = KdlEvent::Kind::Property, .name = "rel", .value = "stylesheet" },
        { .kind = KdlEvent::Kind::Property, .name = "href", .value = "/styles/global.css" },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::EndNode },
        { .kind = KdlEvent::Kind::StartNode, .name = "body" },
        { .kind = KdlEvent::Kind::StartNode, .name = "main" },
        { .kind = KdlEvent::Kind::StartNode, .name = "header" },
        { .kind = KdlEvent::Kind::Property, .name = "class", .value = "py-10 bg-gray-300" },
        { .kind = KdlEvent::Kind::StartNode, .name = "h1" },
        { .kind = KdlEvent::Kind::Property, .name = "class", .value = "text-4xl text-center" },
        { .kind = KdlEvent::Kind::Argument, .value = "kdl - The KDL Document Language" },
        { .kind = KdlEvent::Kind::EndNode }, // h1
        { .kind = KdlEvent::Kind::EndNode }, // header
        { .kind = KdlEvent::Kind::StartNode, .name = "section" },
        { .kind = KdlEvent::Kind::Property, .name = "class", .value = "kdl-section" },
        { .kind = KdlEvent::Kind::Property, .name = "id", .value = "description" },
        { .kind = KdlEvent::Kind::StartNode, .name = "p" },
        { .kind = KdlEvent::Kind::StartNode, .name = "-" },
        { .kind = KdlEvent::Kind::Argument, .value = "kdl is a document language, mostly based on " },
        { .kind = KdlEvent::Kind::EndNode }, // -
        { .kind = KdlEvent::Kind::StartNode, .name = "a" },
        { .kind = KdlEvent::Kind::Property, .name = "href", .value = "https://sdlang.org" },
        { .kind = KdlEvent::Kind::Argument, .value = "SDLang" },
        { .kind = KdlEvent::Kind::EndNode }, // a
        { .kind = KdlEvent::Kind::StartNode, .name = "-" },
        { .kind = KdlEvent::Kind::Argument, .value = " with xml-like semantics that looks like you're invoking a bunch of CLI commands" },
        { .kind = KdlEvent::Kind::EndNode }, // -
        { .kind = KdlEvent::Kind::EndNode }, // section
        { .kind = KdlEvent::Kind::StartNode, .name = "p" },
        { .kind = KdlEvent::Kind::Argument, .value = "It's meant to be used both as a serialization format and a configuration language, and is relatively light on syntax compared to XML." },
        { .kind = KdlEvent::Kind::EndNode }, // p
        { .kind = KdlEvent::Kind::EndNode }, // section
        { .kind = KdlEvent::Kind::StartNode, .name = "section" },
        { .kind = KdlEvent::Kind::Property, .name = "class", .value = "kdl-section" },
        { .kind = KdlEvent::Kind::Property, .name = "id", .value = "design-and-discussion" },
        { .kind = KdlEvent::Kind::StartNode, .name = "h2" },
        { .kind = KdlEvent::Kind::Argument, .value = "Design and Discussion" },
        { .kind = KdlEvent::Kind::EndNode }, // h2
        { .kind = KdlEvent::Kind::StartNode, .name = "p" },
        { .kind = KdlEvent::Kind::StartNode, .name = "-" },
        { .kind = KdlEvent::Kind::Argument, .value = "kdl is still extremely new, and discussion about the format should happen over on the " },
        { .kind = KdlEvent::Kind::EndNode }, // -
        { .kind = KdlEvent::Kind::StartNode, .name = "a" },
        { .kind = KdlEvent::Kind::Property, .name = "href", .value = "https://github.com/kdoclang/kdl/discussions" },
        { .kind = KdlEvent::Kind::StartNode, .name = "-" },
        { .kind = KdlEvent::Kind::Argument, .value = "discussions" },
        { .kind = KdlEvent::Kind::EndNode }, // -
        { .kind = KdlEvent::Kind::EndNode }, // a
        { .kind = KdlEvent::Kind::StartNode, .name = "-" },
        { .kind = KdlEvent::Kind::Argument, .value = " page in the Github repo. Feel free to jump in and give us your 2 cents!" },
        { .kind = KdlEvent::Kind::EndNode }, // -
        { .kind = KdlEvent::Kind::EndNode }, // p
        { .kind = KdlEvent::Kind::EndNode }, // section
        { .kind = KdlEvent::Kind::StartNode, .name = "section" },
        { .kind = KdlEvent::Kind::Property, .name = "class", .value = "kdl-section" },
        { .kind = KdlEvent::Kind::Property, .name = "id", .value = "design-principles" },
        { .kind = KdlEvent::Kind::StartNode, .name = "h2" },
        { .kind = KdlEvent::Kind::Argument, .value = "Design Principles" },
        { .kind = KdlEvent::Kind::EndNode }, // h2
        { .kind = KdlEvent::Kind::StartNode, .name = "ol" },
        { .kind = KdlEvent::Kind::StartNode, .name = "li" },
        { .kind = KdlEvent::Kind::Argument, .value = "Maintainability" },
        { .kind = KdlEvent::Kind::EndNode }, // li
        { .kind = KdlEvent::Kind::StartNode, .name = "li" },
        { .kind = KdlEvent::Kind::Argument, .value = "Flexibility" },
        { .kind = KdlEvent::Kind::EndNode }, // li
        { .kind = KdlEvent::Kind::StartNode, .name = "li" },
        { .kind = KdlEvent::Kind::Argument, .value = "Cognitive simplicity and Learnability" },
        { .kind = KdlEvent::Kind::EndNode }, // li
        { .kind = KdlEvent::Kind::StartNode, .name = "li" },
        { .kind = KdlEvent::Kind::Argument, .value = "Ease of de/serialization" },
        { .kind = KdlEvent::Kind::EndNode }, // li
        { .kind = KdlEvent::Kind::StartNode, .name = "li" },
        { .kind = KdlEvent::Kind::Argument, .value = "Ease of implementation" },
        { .kind = KdlEvent::Kind::EndNode }, // li
        { .kind = KdlEvent::Kind::EndNode }, // ol
        { .kind = KdlEvent::Kind::EndNode }, // section
        { .kind = KdlEvent::Kind::EndNode }, // main
        { .kind = KdlEvent::Kind::EndNode }, // body
        { .kind = KdlEvent::Kind::EndNode }, // html
        { .kind = KdlEvent::Kind::EndDocument },
    };

    const StringView kdlDoc = GetTestKdlDocument();
    Validate(kdlDoc, expected);
}
