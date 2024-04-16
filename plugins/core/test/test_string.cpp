// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/string.h"
#include "he/core/string_fmt.h"

#include "he/core/fmt.h"
#include "he/core/allocator.h"
#include "he/core/memory_ops.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Constants)
{
    // Changing these are potentially breaking so checking them here so a change is made with thoughtfulness.
    static_assert(IsSame<String::ElementType, char>);
    static_assert(String::MaxEmbedCharacters == 31);
    static_assert(String::MaxHeapCharacters == 0x7ffffffe);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Construct)
{
    {
        String s;
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 0);
    }

    {
        String s("");
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 0);
    }

    {
        String s("testing");
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 7);
    }

    {
        String s("testing", 2);
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Construct_Copy)
{
    constexpr char SmallString[] = "testing";
    constexpr uint32_t SmallStringLen = HE_LENGTH_OF(SmallString) - 1;

    String small(SmallString);
    HE_EXPECT(small.IsEmbedded());
    HE_EXPECT_EQ(small.Size(), SmallStringLen);
    HE_EXPECT_EQ_STR(small.Data(), SmallString);

    {
        String copy(small);
        HE_EXPECT(copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), small.Data());
        HE_EXPECT_EQ_STR(copy.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        String copy(small, a2);
        HE_EXPECT(copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), small.Data());
        HE_EXPECT_EQ_STR(copy.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    {
        String copy(small);
        HE_EXPECT(copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), small.Data());
        HE_EXPECT_EQ_STR(copy.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &small.GetAllocator());
    }

    {
        String copy = small;
        HE_EXPECT(copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), small.Data());
        HE_EXPECT_EQ_STR(copy.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &small.GetAllocator());
    }

    constexpr char LargeString[] = "this is a really long string that will certainly go into heap storage, because it is so long.";
    constexpr uint32_t LargeStringLen = HE_LENGTH_OF(LargeString) - 1;

    String large(LargeString);
    HE_EXPECT(!large.IsEmbedded());
    HE_EXPECT_EQ(large.Size(), LargeStringLen);
    HE_EXPECT_EQ_STR(large.Data(), LargeString);

    {
        String copy(large);
        HE_EXPECT(!copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), large.Data());
        HE_EXPECT_EQ_STR(copy.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        String copy(large, a2);
        HE_EXPECT(!copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), large.Data());
        HE_EXPECT_EQ_STR(copy.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    {
        String copy(large);
        HE_EXPECT(!copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), large.Data());
        HE_EXPECT_EQ_STR(copy.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &large.GetAllocator());
    }

    {
        String copy = large;
        HE_EXPECT(!copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), large.Data());
        HE_EXPECT_EQ_STR(copy.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &large.GetAllocator());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Construct_Move)
{
    constexpr char SmallString[] = "testing";
    constexpr uint32_t SmallStringLen = HE_LENGTH_OF(SmallString) - 1;

    String small(SmallString);
    HE_EXPECT(small.IsEmbedded());
    HE_EXPECT_EQ(small.Size(), SmallStringLen);
    HE_EXPECT_EQ_STR(small.Data(), SmallString);

    {
        String moved(Move(small));
        HE_EXPECT(moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), small.Data());
        HE_EXPECT_EQ_STR(moved.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        String moved(Move(small), a2);
        HE_EXPECT(moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), small.Data());
        HE_EXPECT_EQ_STR(moved.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
    }

    {
        String moved(Move(small));
        HE_EXPECT(moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), small.Data());
        HE_EXPECT_EQ_STR(moved.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &small.GetAllocator());
    }

    {
        String moved = Move(small);
        HE_EXPECT(moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), small.Data());
        HE_EXPECT_EQ_STR(moved.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &small.GetAllocator());
    }

    constexpr char LargeString[] = "this is a really long string that will certainly go into heap storage, because it is so long.";
    constexpr uint32_t LargeStringLen = HE_LENGTH_OF(LargeString) - 1;

    {
        String large(LargeString);
        HE_EXPECT(!large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(large.Data(), LargeString);

        String moved(Move(large));
        HE_EXPECT(!moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());

        HE_EXPECT(large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), 0);
    }

    {
        String large(LargeString);
        HE_EXPECT(!large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(large.Data(), LargeString);

        AnotherAllocator a2;
        String moved(Move(large), a2);
        HE_EXPECT(!moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);

        HE_EXPECT(!large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), LargeStringLen);
    }

    {
        String large(LargeString);
        HE_EXPECT(!large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(large.Data(), LargeString);

        String moved(Move(large));
        HE_EXPECT(!moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &large.GetAllocator());

        HE_EXPECT(large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), 0);
    }

    {
        String large(LargeString);
        HE_EXPECT(!large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(large.Data(), LargeString);

        String moved = Move(large);
        HE_EXPECT(!moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &large.GetAllocator());

        HE_EXPECT(large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_assign_copy)
{
    constexpr char SmallString[] = "testing";
    constexpr uint32_t SmallStringLen = HE_LENGTH_OF(SmallString) - 1;

    String small(SmallString);
    HE_EXPECT(small.IsEmbedded());
    HE_EXPECT_EQ(small.Size(), SmallStringLen);
    HE_EXPECT_EQ_STR(small.Data(), SmallString);

    {
        String copy;
        copy = small;
        HE_EXPECT(copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), small.Data());
        HE_EXPECT_EQ_STR(copy.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;

        String copy(a2);
        copy = small;
        HE_EXPECT(copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), small.Data());
        HE_EXPECT_EQ_STR(copy.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    constexpr char LargeString[] = "this is a really long string that will certainly go into heap storage, because it is so long.";
    constexpr uint32_t LargeStringLen = HE_LENGTH_OF(LargeString) - 1;

    String large(LargeString);
    HE_EXPECT(!large.IsEmbedded());
    HE_EXPECT_EQ(large.Size(), LargeStringLen);
    HE_EXPECT_EQ_STR(large.Data(), LargeString);

    {
        String copy;
        copy = large;
        HE_EXPECT(!copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), large.Data());
        HE_EXPECT_EQ_STR(copy.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;

        String copy(a2);
        copy = large;
        HE_EXPECT(!copy.IsEmbedded());
        HE_EXPECT_EQ(copy.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(copy.Data(), large.Data());
        HE_EXPECT_EQ_STR(copy.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_assign_str)
{
    String s("Hello, world!");

    s = "Test";
    HE_EXPECT_EQ(s.Size(), 4);
    HE_EXPECT_EQ(s, "Test");

    s = "bc";
    HE_EXPECT_EQ(s.Size(), 2);
    HE_EXPECT_EQ(s, "bc");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_assign_range)
{
    String s("Hello, world!");

    String s2("hello");
    s = s2;
    HE_EXPECT_EQ(s.Size(), s2.Size());
    HE_EXPECT_EQ(s, s2);

    StringView s3("HELLO!!");
    s = s3;
    HE_EXPECT_EQ(s.Size(), s3.Size());
    HE_EXPECT_EQ(s, s3);

    Vector<char> s4;
    s4.PushBack('a');
    s4.PushBack('o');
    s4.PushBack('c');
    s = s4;
    HE_EXPECT_EQ(s.Size(), s4.Size());
    HE_EXPECT_EQ(s, "aoc");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_assign_move)
{
    constexpr char SmallString[] = "testing";
    constexpr uint32_t SmallStringLen = HE_LENGTH_OF(SmallString) - 1;

    String small(SmallString);
    HE_EXPECT(small.IsEmbedded());
    HE_EXPECT_EQ(small.Size(), SmallStringLen);
    HE_EXPECT_EQ_STR(small.Data(), SmallString);

    {
        String moved;
        moved = Move(small);
        HE_EXPECT(moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), small.Data());
        HE_EXPECT_EQ_STR(moved.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        String moved(a2);
        moved = Move(small);
        HE_EXPECT(moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), SmallStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), small.Data());
        HE_EXPECT_EQ_STR(moved.Data(), SmallString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
    }

    constexpr char LargeString[] = "this is a really long string that will certainly go into heap storage, because it is so long.";
    constexpr uint32_t LargeStringLen = HE_LENGTH_OF(LargeString) - 1;

    {
        String large(LargeString);
        HE_EXPECT(!large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(large.Data(), LargeString);

        String moved;
        moved = Move(large);
        HE_EXPECT(!moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());

        HE_EXPECT(large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), 0);
    }

    {
        String large(LargeString);
        HE_EXPECT(!large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(large.Data(), LargeString);

        AnotherAllocator a2;
        String moved(a2);
        moved = Move(large);
        HE_EXPECT(!moved.IsEmbedded());
        HE_EXPECT_EQ(moved.Size(), LargeStringLen);
        HE_EXPECT_EQ_STR(moved.Data(), LargeString);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);

        HE_EXPECT(!large.IsEmbedded());
        HE_EXPECT_EQ(large.Size(), LargeStringLen);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_index)
{
    String s("test");
    HE_EXPECT_EQ(s[0], 't');
    HE_EXPECT_EQ(s[3], 't');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_plus_equal)
{
    String s;
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");

    s += "Hello";
    HE_EXPECT_EQ(s.Size(), 5);
    HE_EXPECT_EQ_STR(s.Data(), "Hello");

    s += ", ";
    HE_EXPECT_EQ(s.Size(), 7);
    HE_EXPECT_EQ_STR(s.Data(), "Hello, ");

    s += "world!";
    HE_EXPECT_EQ(s.Size(), 13);
    HE_EXPECT_EQ_STR(s.Data(), "Hello, world!");

    s += ' ';
    HE_EXPECT_EQ(s.Size(), 14);
    HE_EXPECT_EQ_STR(s.Data(), "Hello, world! ");

    s += "This is really long to force reallocation onto the heap space of the string object we're testing.";
    HE_EXPECT_GE(s.Capacity(), 111);
    HE_EXPECT_EQ(s.Size(), 111);
    HE_EXPECT_EQ_STR(s.Data(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.");

    StringView v = "Testing view!";
    s += v;
    HE_EXPECT_GE(s.Capacity(), 124);
    HE_EXPECT_EQ(s.Size(), 124);
    HE_EXPECT_EQ_STR(s.Data(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.Testing view!");

    Vector<char> s4;
    s4.PushBack('a');
    s4.PushBack('o');
    s4.PushBack('c');
    s += s4;
    HE_EXPECT_GE(s.Capacity(), 127);
    HE_EXPECT_EQ(s.Size(), 127);
    HE_EXPECT_EQ_STR(s.Data(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.Testing view!aoc");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_plus)
{
    const String s;
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");

    const String s0 = s + "Hello";
    HE_EXPECT_EQ(s0.Size(), 5);
    HE_EXPECT_EQ_STR(s0.Data(), "Hello");

    const String s1 = s0 + ", ";
    HE_EXPECT_EQ(s1.Size(), 7);
    HE_EXPECT_EQ_STR(s1.Data(), "Hello, ");

    const String s2 = s1 + "world!";
    HE_EXPECT_EQ(s2.Size(), 13);
    HE_EXPECT_EQ_STR(s2.Data(), "Hello, world!");

    const String s3 = s2 + ' ';
    HE_EXPECT_EQ(s3.Size(), 14);
    HE_EXPECT_EQ_STR(s3.Data(), "Hello, world! ");

    const String s4 = s3 + "This is really long to force reallocation onto the heap space of the string object we're testing.";
    HE_EXPECT_GE(s4.Capacity(), 111);
    HE_EXPECT_EQ(s4.Size(), 111);
    HE_EXPECT_EQ_STR(s4.Data(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.");

    const StringView view = "Testing view!";
    const String s5 = s4 + view;
    HE_EXPECT_GE(s5.Capacity(), 124);
    HE_EXPECT_EQ(s5.Size(), 124);
    HE_EXPECT_EQ_STR(s5.Data(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.Testing view!");

    Vector<char> vec;
    vec.PushBack('a');
    vec.PushBack('o');
    vec.PushBack('c');
    const String s6 = s5 + vec;
    HE_EXPECT_GE(s6.Capacity(), 127);
    HE_EXPECT_EQ(s6.Size(), 127);
    HE_EXPECT_EQ_STR(s6.Data(), "Hello, world! This is really long to force reallocation onto the heap space of the string object we're testing.Testing view!aoc");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_eq)
{
    const String a("Hello, world!");
    const String b("Hello, world!");
    const String c("Goodbye, world!");

    HE_EXPECT((a == a), a, a);
    HE_EXPECT((a == b), b, a);
    HE_EXPECT(!(a == c), c, a);

    HE_EXPECT((b == a), b, a);
    HE_EXPECT((b == b), b, b);
    HE_EXPECT(!(b == c), b, c);

    HE_EXPECT(!(c == a), c, a);
    HE_EXPECT(!(c == b), c, b);
    HE_EXPECT((c == c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_eq_str)
{
    const String a("Hello, world!");

    HE_EXPECT((a == "Hello, world!"), a);
    HE_EXPECT(!(a == "Goodbye, world!"), a);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_ne)
{
    const String a("Hello, world!");
    const String b("Hello, world!");
    const String c("Goodbye, world!");

    HE_EXPECT(!(a != a), a, a);
    HE_EXPECT(!(a != b), b, a);
    HE_EXPECT((a != c), c, a);

    HE_EXPECT(!(b != a), b, a);
    HE_EXPECT(!(b != b), b, b);
    HE_EXPECT((b != c), b, c);

    HE_EXPECT((c != a), c, a);
    HE_EXPECT((c != b), c, b);
    HE_EXPECT(!(c != c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_ne_str)
{
    const String a("Hello, world!");

    HE_EXPECT(!(a != "Hello, world!"), a);
    HE_EXPECT((a != "Goodbye, world!"), a);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_lt)
{
    const String a("Hello, world!");
    const String b("Hello, world!");
    const String c("Goodbye, world!");

    HE_EXPECT(!(a < a), a, a);
    HE_EXPECT(!(a < b), b, a);
    HE_EXPECT(!(a < c), c, a);

    HE_EXPECT(!(b < a), b, a);
    HE_EXPECT(!(b < b), b, b);
    HE_EXPECT(!(b < c), b, c);

    HE_EXPECT((c < a), c, a);
    HE_EXPECT((c < b), c, b);
    HE_EXPECT(!(c < c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_le)
{
    const String a("Hello, world!");
    const String b("Hello, world!");
    const String c("Goodbye, world!");

    HE_EXPECT((a <= a), a, a);
    HE_EXPECT((a <= b), b, a);
    HE_EXPECT(!(a <= c), c, a);

    HE_EXPECT((b <= a), b, a);
    HE_EXPECT((b <= b), b, b);
    HE_EXPECT(!(b <= c), b, c);

    HE_EXPECT((c <= a), c, a);
    HE_EXPECT((c <= b), c, b);
    HE_EXPECT((c <= c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_gt)
{
    const String a("Hello, world!");
    const String b("Hello, world!");
    const String c("Goodbye, world!");

    HE_EXPECT(!(a > a), a, a);
    HE_EXPECT(!(a > b), b, a);
    HE_EXPECT((a > c), c, a);

    HE_EXPECT(!(b > a), b, a);
    HE_EXPECT(!(b > b), b, b);
    HE_EXPECT((b > c), b, c);

    HE_EXPECT(!(c > a), c, a);
    HE_EXPECT(!(c > b), c, b);
    HE_EXPECT(!(c > c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, operator_ge)
{
    const String a("Hello, world!");
    const String b("Hello, world!");
    const String c("Goodbye, world!");

    HE_EXPECT((a >= a), a, a);
    HE_EXPECT((a >= b), b, a);
    HE_EXPECT((a >= c), c, a);

    HE_EXPECT((b >= a), b, a);
    HE_EXPECT((b >= b), b, b);
    HE_EXPECT((b >= c), b, c);

    HE_EXPECT(!(c >= a), c, a);
    HE_EXPECT(!(c >= b), c, b);
    HE_EXPECT((c >= c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, IsEmbedded)
{
    String s;
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);
    HE_EXPECT_EQ(s.Size(), 0);

    char embedSizedStr[String::MaxEmbedCharacters + 1];
    MemSet(embedSizedStr, 'a', String::MaxEmbedCharacters);
    embedSizedStr[String::MaxEmbedCharacters] = '\0';

    s += embedSizedStr;
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);

    s += 'b';
    HE_EXPECT(!s.IsEmbedded());
    HE_EXPECT_GT(s.Capacity(), String::MaxEmbedCharacters);
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters + 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, IsEmpty)
{
    String s;
    HE_EXPECT(s.IsEmpty());

    char embedSizedStr[String::MaxEmbedCharacters + 1];
    MemSet(embedSizedStr, 'a', String::MaxEmbedCharacters);
    embedSizedStr[String::MaxEmbedCharacters] = '\0';

    s += embedSizedStr;
    HE_EXPECT(!s.IsEmpty());

    s += 'b';
    HE_EXPECT(!s.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Capacity)
{
    String s;
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
HE_TEST(core, string, Size)
{
    String s;
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
HE_TEST(core, string, Reserve)
{
    String s;
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
HE_TEST(core, string, Resize)
{
    // test size changes from embed -> heap
    {
        char zeroes[String::MaxEmbedCharacters + 1]{};

        String s;
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 0);

        s.Resize(16);
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 16);
        HE_EXPECT_EQ_MEM(s.Data(), zeroes, s.Size());

        s.Resize(String::MaxEmbedCharacters);
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);
        HE_EXPECT_EQ_MEM(s.Data(), zeroes, s.Size());

        s.Resize(String::MaxEmbedCharacters + 1);
        HE_EXPECT(!s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters + 1);
        HE_EXPECT_EQ_MEM(s.Data(), zeroes, s.Size());

        s.Resize(String::MaxEmbedCharacters);
        HE_EXPECT(!s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);
        HE_EXPECT_EQ_MEM(s.Data(), zeroes, s.Size());
    }

    // test setting characters
    {
        char expected[String::MaxEmbedCharacters + 1];
        for (uint32_t i = 0; i < 4; ++i)
            expected[i] = 'a';
        for (uint32_t i = 4; i < String::MaxEmbedCharacters; ++i)
            expected[i] = 'b';
        for (uint32_t i = String::MaxEmbedCharacters; i < String::MaxEmbedCharacters + 1; ++i)
            expected[i] = 'c';

        String s;
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 0);

        s.Resize(4, 'a');
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 4);
        HE_EXPECT_EQ_MEM(s.Data(), expected, s.Size());

        s.Resize(String::MaxEmbedCharacters, 'b');
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);
        HE_EXPECT_EQ_MEM(s.Data(), expected, s.Size());

        s.Resize(String::MaxEmbedCharacters + 1, 'c');
        HE_EXPECT(!s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters + 1);
        HE_EXPECT_EQ_MEM(s.Data(), expected, s.Size());
    }

    // test default init
    {
        String s;
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 0);

        s.Resize(4, DefaultInit);
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), 4);

        s.Resize(String::MaxEmbedCharacters, DefaultInit);
        HE_EXPECT(s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);

        s.Resize(String::MaxEmbedCharacters + 1, DefaultInit);
        HE_EXPECT(!s.IsEmbedded());
        HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters + 1);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, ShrinkToFit)
{
    String s;
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);

    s.ShrinkToFit();
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);

    s.Resize(String::MaxEmbedCharacters);
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);

    s.ShrinkToFit();
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);

    s.Resize(String::MaxEmbedCharacters * 2);
    HE_EXPECT(!s.IsEmbedded());
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters * 2);
    HE_EXPECT_GE(s.Capacity(), String::MaxEmbedCharacters * 2);

    s.ShrinkToFit();
    HE_EXPECT(!s.IsEmbedded());
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters * 2);
    HE_EXPECT_GE(s.Capacity(), String::MaxEmbedCharacters * 2);

    s.Resize(String::MaxEmbedCharacters);
    HE_EXPECT(!s.IsEmbedded());
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);
    HE_EXPECT_GE(s.Capacity(), String::MaxEmbedCharacters * 2);

    s.ShrinkToFit();
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT_EQ(s.Size(), String::MaxEmbedCharacters);
    HE_EXPECT_EQ(s.Capacity(), String::MaxEmbedCharacters);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Data)
{
    String s;
    HE_EXPECT_EQ_PTR(s.Data(), StringTestAttorney::GetEmbed(s));

    s.Resize(String::MaxEmbedCharacters);
    HE_EXPECT_EQ_PTR(s.Data(), StringTestAttorney::GetEmbed(s));

    s.Resize(String::MaxEmbedCharacters + 1);
    HE_EXPECT_EQ_PTR(s.Data(), StringTestAttorney::GetHeap(s).data);

    s.Resize(String::MaxEmbedCharacters);
    s.ShrinkToFit();
    HE_EXPECT_EQ_PTR(s.Data(), StringTestAttorney::GetEmbed(s));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Front_Back)
{
    {
        String s("test");
        HE_EXPECT_EQ(s.Front(), 't');
        HE_EXPECT_EQ(s.Back(), 't');
    }

    {
        String s("This is a really long string that should cause the string to allocate the necessary storage on the heap.");
        HE_EXPECT_EQ(s.Front(), 'T');
        HE_EXPECT_EQ(s.Back(), '.');
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, GetAllocator)
{
    String s;
    HE_EXPECT_EQ_PTR(&s.GetAllocator(), &Allocator::GetDefault());

    AnotherAllocator a2;
    String s2(a2);
    HE_EXPECT_EQ_PTR(&s2.GetAllocator(), &a2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, HashCode)
{
    HE_EXPECT_EQ(String("foobar").HashCode(), 0x0b9d35b96e1f6fe2);
    HE_EXPECT_EQ(String("Hello, world!").HashCode(), 0xf83c9721fa2a9cdc);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Substring)
{
    const String s = "Hello, world!";

    {
        const String sub = s.Substring(0);
        HE_EXPECT_EQ(sub.Size(), s.Size());
        HE_EXPECT_EQ_STR(sub.Data(), s.Data());
    }

    {
        const String sub = s.Substring(2);
        HE_EXPECT_EQ(sub.Size(), s.Size() - 2);
        HE_EXPECT_EQ_STR(sub.Data(), s.Data() + 2);
    }

    {
        const String sub = s.Substring(0, 0);
        HE_EXPECT_EQ(sub.Size(), 0);
    }

    {
        const String sub = s.Substring(0, s.Size());
        HE_EXPECT_EQ(sub.Size(), s.Size());
        HE_EXPECT_EQ_STR(sub.Data(), s.Data());
    }

    {
        const String sub = s.Substring(2, 5);
        HE_EXPECT_EQ(sub.Size(), 5);
        HE_EXPECT_EQ(sub, (StringView{ s.Data() + 2, 5 }));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Find)
{
    const String haystack = "Hello, world!";

    HE_EXPECT_EQ_PTR(haystack.Find('z'), nullptr);
    HE_EXPECT_EQ_PTR(haystack.Find('o'), (haystack.Data() + 4));
    HE_EXPECT_EQ_PTR(haystack.Find('!'), (haystack.Data() + 12));

    HE_EXPECT_EQ_PTR(haystack.Find("Hello!"), nullptr);
    HE_EXPECT_EQ_PTR(haystack.Find("world"), (haystack.Data() + 7));

    HE_EXPECT_EQ_PTR(haystack.Find("Hello!", 5), haystack.Data());
    HE_EXPECT_EQ_PTR(haystack.Find("wor12ld", 3), (haystack.Data() + 7));

    HE_EXPECT_EQ_PTR(haystack.Find(StringView("Hello!")), nullptr);
    HE_EXPECT_EQ_PTR(haystack.Find(StringView("world")), (haystack.Data() + 7));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, FindIndex)
{
    const String haystack = "Hello, world!";

    HE_EXPECT_EQ(haystack.FindIndex('z'), static_cast<uint32_t>(-1));
    HE_EXPECT_EQ(haystack.FindIndex('o'), 4);
    HE_EXPECT_EQ(haystack.FindIndex('!'), 12);

    HE_EXPECT_EQ(haystack.FindIndex("Hello!"), static_cast<uint32_t>(-1));
    HE_EXPECT_EQ(haystack.FindIndex("world"), 7);

    HE_EXPECT_EQ(haystack.FindIndex("Hello!", 5), 0);
    HE_EXPECT_EQ(haystack.FindIndex("wor12ld", 3), 7);

    HE_EXPECT_EQ(haystack.FindIndex(StringView("Hello!")), static_cast<uint32_t>(-1));
    HE_EXPECT_EQ(haystack.FindIndex(StringView("world")), 7);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Contains)
{
    const String haystack = "Hello, world!";

    HE_EXPECT(!haystack.Contains('z'));
    HE_EXPECT(haystack.Contains('o'));
    HE_EXPECT(haystack.Contains('!'));

    HE_EXPECT(!haystack.Contains("Hello!"));
    HE_EXPECT(haystack.Contains("world"));

    HE_EXPECT(haystack.Contains("Hello!", 5));
    HE_EXPECT(haystack.Contains("wor12ld", 3));

    HE_EXPECT(!haystack.Contains(StringView("Hello!")));
    HE_EXPECT(haystack.Contains(StringView("world")));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, CompareTo)
{
    const String a("Hello, world!");
    const String b("Hello, World!");
    const String c("Goodbye, world!");
    const StringView v("Another one");

    HE_EXPECT_EQ(a.CompareTo(a), 0);
    HE_EXPECT_GT(a.CompareTo(b), 0);
    HE_EXPECT_GT(a.CompareTo(c), 0);

    HE_EXPECT_LT(b.CompareTo(a), 0);
    HE_EXPECT_EQ(b.CompareTo(b), 0);
    HE_EXPECT_GT(b.CompareTo(c), 0);

    HE_EXPECT_LT(c.CompareTo(a), 0);
    HE_EXPECT_LT(c.CompareTo(b), 0);
    HE_EXPECT_EQ(c.CompareTo(c), 0);

    HE_EXPECT_GT(c.CompareTo(v), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, CompareToI)
{
    const String a("Hello, world!");
    const String b("Hello, World!");
    const String c("Goodbye, world!");
    const StringView v("Another one");

    HE_EXPECT_EQ(a.CompareToI(a), 0);
    HE_EXPECT_EQ(a.CompareToI(b), 0);
    HE_EXPECT_GT(a.CompareToI(c), 0);

    HE_EXPECT_EQ(b.CompareToI(a), 0);
    HE_EXPECT_EQ(b.CompareToI(b), 0);
    HE_EXPECT_GT(b.CompareToI(c), 0);

    HE_EXPECT_LT(c.CompareToI(a), 0);
    HE_EXPECT_LT(c.CompareToI(b), 0);
    HE_EXPECT_EQ(c.CompareToI(c), 0);

    HE_EXPECT_GT(c.CompareToI(v), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Begin)
{
    {
        String s;
        HE_EXPECT(s.Begin() == StringTestAttorney::GetEmbed(s));
    }

    {
        String s("Hello, world!");
        HE_EXPECT_EQ_PTR(s.Begin(), StringTestAttorney::GetEmbed(s));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, End)
{
    {
        String s;
        HE_EXPECT(s.End() == StringTestAttorney::GetEmbed(s));
    }

    {
        String s("Hello, world!");
        HE_EXPECT_EQ_PTR(s.End(), (StringTestAttorney::GetEmbed(s) + s.Size()));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, RangeBasedFor)
{
    constexpr char TestStr[] = "Hello, world!";

    String s(TestStr);
    uint32_t i = 0;
    for (char c : s)
    {
        HE_EXPECT_EQ(c, TestStr[i++]);
    }
    HE_EXPECT_EQ(i, HE_LENGTH_OF(TestStr) - 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Clear)
{
    String s;
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT(s.IsEmpty());

    s.Clear();
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT(s.IsEmpty());

    s.Reserve(16);
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT(s.IsEmpty());

    s.Resize(10);
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT(!s.IsEmpty());

    s.Clear();
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT(s.IsEmpty());

    s += "testing a small string";
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT(!s.IsEmpty());

    s.Clear();
    HE_EXPECT(s.IsEmbedded());
    HE_EXPECT(s.IsEmpty());

    s += "testing a long string that will force the storage onto the heap so we can test Clear()";
    HE_EXPECT(!s.IsEmbedded());
    HE_EXPECT(!s.IsEmpty());

    s.Clear();
    HE_EXPECT(!s.IsEmbedded());
    HE_EXPECT(s.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Insert)
{
    String s;
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");

    s.Insert(0, 'a');
    HE_EXPECT_EQ(s.Size(), 1);
    HE_EXPECT_EQ_STR(s.Data(), "a");

    s.Insert(1, 'b');
    HE_EXPECT_EQ(s.Size(), 2);
    HE_EXPECT_EQ_STR(s.Data(), "ab");

    s.Insert(1, "zx");
    HE_EXPECT_EQ(s.Size(), 4);
    HE_EXPECT_EQ_STR(s.Data(), "azxb");

    s.Insert(3, "12345", 2);
    HE_EXPECT_EQ(s.Size(), 6);
    HE_EXPECT_EQ_STR(s.Data(), "azx12b");

    constexpr char Test[] = "possible";
    constexpr uint32_t TestLen = HE_LENGTH_OF(Test) - 1;

    s.Insert(0, Test, Test + TestLen);
    HE_EXPECT_EQ(s.Size(), 14);
    HE_EXPECT_EQ_STR(s.Data(), "possibleazx12b");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Erase)
{
    String s("abcdefg");
    HE_EXPECT_EQ(s.Size(), 7);
    HE_EXPECT_EQ_STR(s.Data(), "abcdefg");

    s.Erase(0, 1);
    HE_EXPECT_EQ(s.Size(), 6);
    HE_EXPECT_EQ_STR(s.Data(), "bcdefg");

    s.Erase(3, 3);
    HE_EXPECT_EQ(s.Size(), 3);
    HE_EXPECT_EQ_STR(s.Data(), "bcd");

    s.Erase(1, 1);
    HE_EXPECT_EQ(s.Size(), 2);
    HE_EXPECT_EQ_STR(s.Data(), "bd");

    s.Erase(0, 2);
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, PushBack)
{
    String s;
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");

    s.PushBack('a');
    HE_EXPECT_EQ(s.Size(), 1);
    HE_EXPECT_EQ_STR(s.Data(), "a");

    s.PushBack('b');
    HE_EXPECT_EQ(s.Size(), 2);
    HE_EXPECT_EQ_STR(s.Data(), "ab");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, PushFront)
{
    String s;
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");

    s.PushFront('a');
    HE_EXPECT_EQ(s.Size(), 1);
    HE_EXPECT_EQ_STR(s.Data(), "a");

    s.PushFront('b');
    HE_EXPECT_EQ(s.Size(), 2);
    HE_EXPECT_EQ_STR(s.Data(), "ba");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, PopBack)
{
    String s("ab");
    HE_EXPECT_EQ(s.Size(), 2);
    HE_EXPECT_EQ_STR(s.Data(), "ab");

    const char c0 = s.PopBack();
    HE_EXPECT_EQ(c0, 'b');
    HE_EXPECT_EQ(s.Size(), 1);
    HE_EXPECT_EQ_STR(s.Data(), "a");

    const char c1 = s.PopBack();
    HE_EXPECT_EQ(c1, 'a');
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, PopFront)
{
    String s("ab");
    HE_EXPECT_EQ(s.Size(), 2);
    HE_EXPECT_EQ_STR(s.Data(), "ab");

    const char c0 = s.PopFront();
    HE_EXPECT_EQ(c0, 'a');
    HE_EXPECT_EQ(s.Size(), 1);
    HE_EXPECT_EQ_STR(s.Data(), "b");

    const char c1 = s.PopFront();
    HE_EXPECT_EQ(c1, 'b');
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Append)
{
    String s;
    HE_EXPECT_EQ(s.Size(), 0);
    HE_EXPECT_EQ_STR(s.Data(), "");

    s.Append('a');
    HE_EXPECT_EQ(s.Size(), 1);
    HE_EXPECT_EQ_STR(s.Data(), "a");

    s.Append("bc");
    HE_EXPECT_EQ(s.Size(), 3);
    HE_EXPECT_EQ_STR(s.Data(), "abc");

    s.Append("12345", 2);
    HE_EXPECT_EQ(s.Size(), 5);
    HE_EXPECT_EQ_STR(s.Data(), "abc12");

    String s2("hello");
    s.Append(s2);
    HE_EXPECT_EQ(s.Size(), 10);
    HE_EXPECT_EQ_STR(s.Data(), "abc12hello");

    constexpr char Test[] = "possible";
    constexpr uint32_t TestLen = HE_LENGTH_OF(Test) - 1;

    s.Append(Test, Test + TestLen);
    HE_EXPECT_EQ(s.Size(), 18);
    HE_EXPECT_EQ_STR(s.Data(), "abc12hellopossible");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Assign)
{
    String s("Hello, world!");

    s.Assign("Test");
    HE_EXPECT_EQ(s.Size(), 4);
    HE_EXPECT_EQ_STR(s.Data(), "Test");

    s.Assign("bc");
    HE_EXPECT_EQ(s.Size(), 2);
    HE_EXPECT_EQ_STR(s.Data(), "bc");

    String s2("hello");
    s.Assign(s2);
    HE_EXPECT_EQ(s.Size(), 5);
    HE_EXPECT_EQ_STR(s.Data(), "hello");

    constexpr char Test[] = "possible";
    constexpr uint32_t TestLen = HE_LENGTH_OF(Test) - 1;

    s.Assign(Test, Test + TestLen);
    HE_EXPECT_EQ(s.Size(), TestLen);
    HE_EXPECT_EQ_STR(s.Data(), Test);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Replace)
{
    // TODO:
    /*
    uint32_t Replace(const char* search, uint32_t searchLen, const char* replacement, uint32_t replacementLen);

    uint32_t Replace(const char* search, const char* replacement) { return Replace(search, StrLen(search), replacement, StrLen(replacement)); }

    template <ContiguousRangeOf<const char> R>
    uint32_t Replace(const R & search, const char* replacement) { return Replace(search.Data(), search.Size(), replacement, StrLen(replacement)); }

    template <ContiguousRangeOf<const char> R>
    uint32_t Replace(const char* search, const R & replacement) { return Replace(search, StrLen(search), replacement.Data(), replacement.Size()); }

    template <ContiguousRangeOf<const char> R1, ContiguousRangeOf<const char> R2>
    uint32_t Replace(const R1 & search, const R2 & replacement) { return Replace(search.Data(), search.Size(), replacement.Data(), replacement.Size()); }
    */
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, fmt)
{
    constexpr char TestString[] = "Hello, world!";

    const String s1(TestString);

    const String s2 = Format("{}", s1);
    HE_EXPECT_EQ(s1, s2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, literals)
{
    {
        const String value = "abc\0\0def";
        HE_EXPECT_EQ(value.Size(), 3);
        HE_EXPECT_EQ(value, "abc");
    }

    {
        const String value = "abc\0\0def"_s;
        HE_EXPECT_EQ(value.Size(), 8);
        HE_EXPECT_EQ_MEM(value.Data(), "abc\0\0def", 8);
    }
}
