// Copyright Chad Engler

#include "he/core/string_writer.h"

#include "fixtures.h"

#include "he/core/allocator.h"
#include "he/core/string_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, string_writer, Construct)
{
    String buf;
    StringWriter writer(buf);
    HE_EXPECT(writer.IsEmpty());
    HE_EXPECT_EQ(writer.Capacity(), String::MaxEmbedCharacters);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, Construct_Copy)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    String buf;

    StringWriter writer(buf);
    writer.Write(TestStr);
    HE_EXPECT_EQ(writer.Size(), TestStrLen);
    HE_EXPECT_EQ(writer.Str(), TestStr);

    {
        StringWriter copy(writer);
        HE_EXPECT_EQ(copy.Size(), writer.Size());
        HE_EXPECT_EQ(copy.Str(), writer.Str());
    }

    {
        StringWriter copy = writer;
        HE_EXPECT_EQ(copy.Size(), writer.Size());
        HE_EXPECT_EQ(copy.Str(), writer.Str());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, Construct_Move)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    {
        String buf;
        StringWriter writer(buf);
        writer.Write(TestStr);
        HE_EXPECT_EQ(writer.Size(), TestStrLen);
        HE_EXPECT_EQ(writer.Str(), TestStr);
        const char* ptr = writer.Str().Data();

        StringWriter moved(Move(writer));
        HE_EXPECT_EQ(moved.Size(), TestStrLen);
        HE_EXPECT_EQ_PTR(moved.Str().Data(), ptr);
        HE_EXPECT_EQ(moved.Str(), TestStr);

        HE_EXPECT(writer.IsEmpty());
    }

    {
        String buf;
        StringWriter writer(buf);
        writer.Write(TestStr);
        HE_EXPECT_EQ(writer.Size(), TestStrLen);
        HE_EXPECT_EQ(writer.Str(), TestStr);
        const char* ptr = writer.Str().Data();

        StringWriter moved = Move(writer);
        HE_EXPECT_EQ(moved.Size(), TestStrLen);
        HE_EXPECT_EQ_PTR(moved.Str().Data(), ptr);
        HE_EXPECT_EQ(moved.Str(), TestStr);

        HE_EXPECT(writer.IsEmpty());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, operator_assign_copy)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    String buf;
    StringWriter writer(buf);
    writer.Write(TestStr);
    HE_EXPECT_EQ(writer.Size(), TestStrLen);
    HE_EXPECT_EQ(writer.Str(), TestStr);

    StringWriter copy(writer);
    copy = writer;
    HE_EXPECT_EQ(copy.Size(), writer.Size());
    HE_EXPECT_EQ(copy.Str(), writer.Str());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, operator_assign_move)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    String buf;
    StringWriter writer(buf);
    writer.Write(TestStr);
    HE_EXPECT_EQ(writer.Size(), TestStrLen);
    HE_EXPECT_EQ(writer.Str(), TestStr);

    StringWriter moved(buf);
    moved = Move(writer);
    HE_EXPECT_EQ(moved.Size(), TestStrLen);
    HE_EXPECT_EQ_PTR(moved.Str().Data(), buf.Data());
    HE_EXPECT_EQ(moved.Str(), TestStr);

    HE_EXPECT(writer.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, operator_index)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    String buf;
    StringWriter writer(buf);
    writer.Write(TestStr);
    HE_EXPECT_EQ(writer.Size(), TestStrLen);

    for (uint32_t i = 0; i < TestStrLen; ++i)
    {
        HE_EXPECT_EQ(writer[i], TestStr[i]);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, operator_plus_equal)
{
    String buf;
    StringWriter s(buf);
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ(s.Str(), "");

    s += "Hello";
    HE_EXPECT_EQ(s.Size(), 5);
    HE_EXPECT_EQ(s.Str(), "Hello");

    s += ", ";
    HE_EXPECT_EQ(s.Size(), 7);
    HE_EXPECT_EQ(s.Str(), "Hello, ");

    s += "world!";
    HE_EXPECT_EQ(s.Size(), 13);
    HE_EXPECT_EQ(s.Str(), "Hello, world!");

    s += ' ';
    HE_EXPECT_EQ(s.Size(), 14);
    HE_EXPECT_EQ(s.Str(), "Hello, world! ");

    s += "This is really long to force reallocation onto the heap space of the string object we're testing.";
    HE_EXPECT_GE(s.Capacity(), 111);
    HE_EXPECT_EQ(s.Size(), 111);
    HE_EXPECT_EQ(s.Str(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.");

    StringView v = "Testing view!";
    s += v;
    HE_EXPECT_GE(s.Capacity(), 124);
    HE_EXPECT_EQ(s.Size(), 124);
    HE_EXPECT_EQ(s.Str(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.Testing view!");

    Vector<char> s4;
    s4.PushBack('a');
    s4.PushBack('o');
    s4.PushBack('c');
    s += s4;
    HE_EXPECT_GE(s.Capacity(), 127);
    HE_EXPECT_EQ(s.Size(), 127);
    HE_EXPECT_EQ(s.Str(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.Testing view!aoc");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, IsEmpty)
{
    String buf;
    StringWriter writer(buf);
    HE_EXPECT(writer.IsEmpty());

    writer.Write('a');
    HE_EXPECT(!writer.IsEmpty());

    writer.Clear();
    HE_EXPECT(writer.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, Capacity)
{
    String buf;
    StringWriter s(buf);
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);

    char embedSizedStr[String::MaxEmbedCharacters + 1];
    MemSet(embedSizedStr, 'a', String::MaxEmbedCharacters);
    embedSizedStr[String::MaxEmbedCharacters] = '\0';

    s += embedSizedStr;
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);

    s += 'b';
    HE_EXPECT_GT(s.Capacity(), String::MaxEmbedCharacters);

    const uint32_t newCap = s.Capacity() * 2;
    s.Reserve(newCap);
    HE_EXPECT_GE(s.Capacity(), newCap);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, Size)
{
    String buf;
    StringWriter s(buf);
    HE_EXPECT_EQ(s.Size(), 0);

    char embedSizedStr[String::MaxEmbedCharacters + 1];
    MemSet(embedSizedStr, 'a', String::MaxEmbedCharacters);
    embedSizedStr[String::MaxEmbedCharacters] = '\0';

    s += embedSizedStr;
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);

    s += 'b';
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters + 1);

    const uint32_t newCap = s.Capacity() * 2;
    s.Reserve(newCap);
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters + 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, Reserve)
{
    String buf;
    StringWriter s(buf);
    HE_EXPECT_EQ(s.Size(), 0);

    char embedSizedStr[String::MaxEmbedCharacters + 1];
    MemSet(embedSizedStr, 'a', String::MaxEmbedCharacters);
    embedSizedStr[String::MaxEmbedCharacters] = '\0';

    s += embedSizedStr;
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);

    s += 'b';
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters + 1);

    const uint32_t newCap = s.Capacity() * 2;
    s.Reserve(newCap);
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters + 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, Str)
{
    String buf;
    StringWriter writer(buf);
    HE_EXPECT_EQ_PTR(writer.Str().Data(), buf.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, Clear)
{
    String buf;
    StringWriter writer(buf);
    HE_EXPECT(writer.IsEmpty());

    writer.Clear();
    HE_EXPECT(writer.IsEmpty());

    writer.Reserve(16);
    HE_EXPECT(writer.IsEmpty());

    writer.Write("1234567890");
    HE_EXPECT(!writer.IsEmpty());

    writer.Clear();
    HE_EXPECT(writer.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, PushBack)
{
    String buf;
    StringWriter writer(buf);

    writer.Clear();
    writer.PushBack('a');
    HE_EXPECT_EQ(writer.Size(), 1);
    HE_EXPECT_EQ(writer.Str(), "a");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, Write)
{
    String buf;
    StringWriter writer(buf);

    // char
    writer.Clear();
    writer.Write('{');
    HE_EXPECT_EQ(writer.Size(), 1);
    HE_EXPECT_EQ(writer.Str(), "{");

    // string
    constexpr const char TestStr[] = "{testing}";
    writer.Clear();
    writer.Write(TestStr);
    HE_EXPECT_EQ(writer.Size(), HE_LENGTH_OF(TestStr) - 1);
    HE_EXPECT_EQ(writer.Str(), TestStr);

    // string view
    constexpr StringView TestStrView = "magic!";
    writer.Clear();
    writer.Write(TestStrView);
    HE_EXPECT_EQ(writer.Size(), TestStrView.Size());
    HE_EXPECT_EQ(writer.Str(), TestStrView);

    // formatted
    writer.Clear();
    writer.Write("My num: {:#018x} {{5}}", 0x1122334455667788ull);
    HE_EXPECT_EQ(writer.Str(), "My num: 0x1122334455667788 {5}");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, WriteIndent)
{
    #define TEST_INDENT "    "

    String buf;
    StringWriter writer(buf);

    HE_EXPECT(writer.IsEmpty());
    writer.WriteIndent();
    HE_EXPECT(writer.IsEmpty());

    writer.IncreaseIndent();
    writer.WriteIndent();
    HE_EXPECT_EQ(writer.Str(), TEST_INDENT);

    writer.IncreaseIndent();
    writer.WriteIndent();
    HE_EXPECT_EQ(writer.Str(), TEST_INDENT TEST_INDENT TEST_INDENT);

    writer.DecreaseIndent();
    writer.WriteIndent();
    HE_EXPECT_EQ(writer.Str(), TEST_INDENT TEST_INDENT TEST_INDENT TEST_INDENT);

    writer.DecreaseIndent();
    writer.WriteIndent();
    HE_EXPECT_EQ(writer.Str(), TEST_INDENT TEST_INDENT TEST_INDENT TEST_INDENT);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_writer, WriteLine)
{
    String buf;
    StringWriter writer(buf);

    writer.WriteLine("line one");
    HE_EXPECT_EQ(writer.Str(), "line one\n");

    writer.IncreaseIndent();
    writer.WriteLine("line two");
    HE_EXPECT_EQ(writer.Str(), "line one\n    line two\n");

    writer.WriteLine("test: {}", 5);
    HE_EXPECT_EQ(writer.Str(), "line one\n    line two\n    test: 5\n");
}
