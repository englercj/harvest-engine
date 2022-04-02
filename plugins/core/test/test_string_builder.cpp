// Copyright Chad Engler

#include "he/core/string_builder.h"

#include "fixtures.h"

#include "he/core/allocator.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, string_builder, Construct)
{
    {
        StringBuilder b;
        HE_EXPECT(b.IsEmpty());
        HE_EXPECT_EQ(b.Capacity(), String::MaxEmbedCharacters);
        HE_EXPECT_EQ_PTR(&b.GetAllocator(), &Allocator::GetDefault());
    }

    {
        Allocator& a = Allocator::GetDefault();
        StringBuilder b(a);
        HE_EXPECT(b.IsEmpty());
        HE_EXPECT_EQ(b.Capacity(), String::MaxEmbedCharacters);
        HE_EXPECT_EQ_PTR(&b.GetAllocator(), &a);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, Construct_Copy)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    StringBuilder buf;
    buf.Write(TestStr);
    HE_EXPECT_EQ(buf.Size(), TestStrLen);
    HE_EXPECT_EQ(buf.Str(), TestStr);

    {
        StringBuilder copy(buf);
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ(copy.Str(), buf.Str());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &buf.GetAllocator());
    }

    {
        AnotherAllocator a2;
        StringBuilder copy(buf, a2);
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ(copy.Str(), buf.Str());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    {
        StringBuilder copy = buf;
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ(copy.Str(), buf.Str());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &buf.GetAllocator());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, Construct_Move)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    {
        StringBuilder buf;
        buf.Write(TestStr);
        HE_EXPECT_EQ(buf.Size(), TestStrLen);
        HE_EXPECT_EQ(buf.Str(), TestStr);
        const char* ptr = buf.Str().Data();

        StringBuilder moved(Move(buf));
        HE_EXPECT_EQ(moved.Size(), TestStrLen);
        HE_EXPECT_NE_PTR(moved.Str().Data(), ptr);
        HE_EXPECT_EQ(moved.Str(), TestStr);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &buf.GetAllocator());

        HE_EXPECT(!buf.IsEmpty());
        HE_EXPECT_EQ_PTR(buf.Str().Data(), ptr);
    }

    {
        StringBuilder buf;
        buf.Write(TestStr);
        HE_EXPECT_EQ(buf.Size(), TestStrLen);
        HE_EXPECT_EQ(buf.Str(), TestStr);
        const char* ptr = buf.Str().Data();

        AnotherAllocator a2;
        StringBuilder moved(Move(buf), a2);
        HE_EXPECT_EQ(moved.Size(), TestStrLen);
        HE_EXPECT_NE_PTR(moved.Str().Data(), ptr);
        HE_EXPECT_EQ(moved.Str(), TestStr);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);

        HE_EXPECT(!buf.IsEmpty());
        HE_EXPECT_EQ_PTR(buf.Str().Data(), ptr);
    }

    {
        StringBuilder buf;
        buf.Write(TestStr);
        HE_EXPECT_EQ(buf.Size(), TestStrLen);
        HE_EXPECT_EQ(buf.Str(), TestStr);
        const char* ptr = buf.Str().Data();

        StringBuilder moved = Move(buf);
        HE_EXPECT_EQ(moved.Size(), TestStrLen);
        HE_EXPECT_NE_PTR(moved.Str().Data(), ptr);
        HE_EXPECT_EQ(moved.Str(), TestStr);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &buf.GetAllocator());

        HE_EXPECT(!buf.IsEmpty());
        HE_EXPECT_EQ_PTR(buf.Str().Data(), ptr);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, operator_assign_copy)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    StringBuilder buf;
    buf.Write(TestStr);
    HE_EXPECT_EQ(buf.Size(), TestStrLen);
    HE_EXPECT_EQ(buf.Str(), TestStr);

    {
        StringBuilder copy;
        copy = buf;
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ(copy.Str(), buf.Str());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        StringBuilder copy(a2);
        copy = buf;
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ(copy.Str(), buf.Str());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, operator_assign_move)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    {
        StringBuilder buf;
        buf.Write(TestStr);
        HE_EXPECT_EQ(buf.Size(), TestStrLen);
        HE_EXPECT_EQ(buf.Str(), TestStr);
        const char* ptr = buf.Str().Data();

        StringBuilder moved;
        moved = Move(buf);
        HE_EXPECT_EQ(moved.Size(), TestStrLen);
        HE_EXPECT_NE_PTR(moved.Str().Data(), ptr);
        HE_EXPECT_EQ(moved.Str(), TestStr);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());

        HE_EXPECT(!buf.IsEmpty());
        HE_EXPECT_EQ_PTR(buf.Str().Data(), ptr);
    }

    {
        StringBuilder buf;
        buf.Write(TestStr);
        HE_EXPECT_EQ(buf.Size(), TestStrLen);
        HE_EXPECT_EQ(buf.Str(), TestStr);
        const char* ptr = buf.Str().Data();

        AnotherAllocator a2;
        StringBuilder moved(a2);
        moved = Move(buf);
        HE_EXPECT_EQ(moved.Size(), TestStrLen);
        HE_EXPECT_NE_PTR(moved.Str().Data(), ptr);
        HE_EXPECT_EQ(moved.Str(), TestStr);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);

        HE_EXPECT(!buf.IsEmpty());
        HE_EXPECT_EQ_PTR(buf.Str().Data(), ptr);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, operator_index)
{
    constexpr char TestStr[] = "Hello";
    constexpr uint32_t TestStrLen = HE_LENGTH_OF(TestStr) - 1;

    StringBuilder buf;
    buf.Write(TestStr);
    HE_EXPECT_EQ(buf.Size(), TestStrLen);

    for (uint32_t i = 0; i < TestStrLen; ++i)
    {
        HE_EXPECT_EQ(buf[i], TestStr[i]);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, operator_plus_equal)
{
    StringBuilder s;
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
HE_TEST(core, string_builder, IsEmpty)
{
    StringBuilder buf;
    HE_EXPECT(buf.IsEmpty());

    buf.Write('a');
    HE_EXPECT(!buf.IsEmpty());

    buf.Clear();
    HE_EXPECT(buf.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, Capacity)
{
    StringBuilder s;
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
HE_TEST(core, string_builder, Size)
{
    StringBuilder s;
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
HE_TEST(core, string_builder, Reserve)
{
    StringBuilder s;
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
HE_TEST(core, string_builder, Str)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, GetAllocator)
{
    StringBuilder b;
    HE_EXPECT_EQ_PTR(&b.GetAllocator(), &Allocator::GetDefault());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, Clear)
{
    StringBuilder buf;
    HE_EXPECT(buf.IsEmpty());

    buf.Clear();
    HE_EXPECT(buf.IsEmpty());

    buf.Reserve(16);
    HE_EXPECT(buf.IsEmpty());

    buf.Write("1234567890");
    HE_EXPECT(!buf.IsEmpty());

    buf.Clear();
    HE_EXPECT(buf.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, PushBack)
{
    StringBuilder buf;

    buf.Clear();
    buf.PushBack('a');
    HE_EXPECT_EQ(buf.Size(), 1);
    HE_EXPECT_EQ(buf.Str(), "a");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, Write)
{
    StringBuilder buf;

    // char
    buf.Clear();
    buf.Write('{');
    HE_EXPECT_EQ(buf.Size(), 1);
    HE_EXPECT_EQ(buf.Str(), "{");

    // string
    constexpr const char TestStr[] = "{testing}";
    buf.Clear();
    buf.Write(TestStr);
    HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(TestStr) - 1);
    HE_EXPECT_EQ(buf.Str(), TestStr);

    // string view
    constexpr StringView TestStrView = "magic!";
    buf.Clear();
    buf.Write(TestStrView);
    HE_EXPECT_EQ(buf.Size(), TestStrView.Size());
    HE_EXPECT_EQ(buf.Str(), TestStrView);

    // formatted
    buf.Clear();
    buf.Write("My num: {:#018x} {{5}}", 0x1122334455667788ull);
    HE_EXPECT_EQ(buf.Str(), "My num: 0x1122334455667788 {5}");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, WriteIndent)
{
    #define TEST_INDENT "    "

    StringBuilder buf;

    HE_EXPECT(buf.IsEmpty());
    buf.WriteIndent();
    HE_EXPECT(buf.IsEmpty());

    buf.IncreaseIndent();
    buf.WriteIndent();
    HE_EXPECT_EQ(buf.Str(), TEST_INDENT);

    buf.IncreaseIndent();
    buf.WriteIndent();
    HE_EXPECT_EQ(buf.Str(), TEST_INDENT TEST_INDENT TEST_INDENT);

    buf.DecreaseIndent();
    buf.WriteIndent();
    HE_EXPECT_EQ(buf.Str(), TEST_INDENT TEST_INDENT TEST_INDENT TEST_INDENT);

    buf.DecreaseIndent();
    buf.WriteIndent();
    HE_EXPECT_EQ(buf.Str(), TEST_INDENT TEST_INDENT TEST_INDENT TEST_INDENT);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_builder, WriteLine)
{
    StringBuilder buf;

    buf.WriteLine("line one");
    HE_EXPECT_EQ(buf.Str(), "line one\n");

    buf.IncreaseIndent();
    buf.WriteLine("line two");
    HE_EXPECT_EQ(buf.Str(), "line one\n    line two\n");

    buf.WriteLine("test: {}", 5);
    HE_EXPECT_EQ(buf.Str(), "line one\n    line two\n    test: 5\n");
}
