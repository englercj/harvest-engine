// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/string.h"
#include "he/core/string_fmt.h"

#include "he/core/appender.h"
#include "he/core/allocator.h"
#include "he/core/memory_ops.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

#include "fmt/format.h"

#include <type_traits>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_IsEmpty)
{
    static_assert(String::IsEmpty(nullptr));
    static_assert(String::IsEmpty(""));
    static_assert(!String::IsEmpty("abc"));

    HE_EXPECT(String::IsEmpty(nullptr));
    HE_EXPECT(String::IsEmpty(""));
    HE_EXPECT(!String::IsEmpty("abc"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_LengthConst)
{
    static_assert(String::LengthConst("") == 0);
    static_assert(String::LengthConst("abc") == 3);

    HE_EXPECT_EQ(String::LengthConst(""), 0);
    HE_EXPECT_EQ(String::LengthConst("abc"), 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_Length)
{
    HE_EXPECT_EQ(String::Length(""), 0);
    HE_EXPECT_EQ(String::Length("abc"), 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_LengthN)
{
    HE_EXPECT_EQ(String::LengthN("", 0), 0);
    HE_EXPECT_EQ(String::LengthN("", 1), 0);
    HE_EXPECT_EQ(String::LengthN("abc", 3), 3);
    HE_EXPECT_EQ(String::LengthN("abc", 1), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_Compare)
{
    HE_EXPECT_EQ(String::Compare("abc", "abc"), 0);
    HE_EXPECT_LT(String::Compare("abc", "acb"), 0);
    HE_EXPECT_GT(String::Compare("cba", "abc"), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_CompareN)
{
    HE_EXPECT_EQ(String::CompareN("abc", "abc", 3), 0);
    HE_EXPECT_LT(String::CompareN("abc", "acb", 3), 0);
    HE_EXPECT_GT(String::CompareN("cba", "abc", 3), 0);

    HE_EXPECT_EQ(String::CompareN("abc", "abc", 2), 0);
    HE_EXPECT_LT(String::CompareN("abc", "acb", 2), 0);
    HE_EXPECT_GT(String::CompareN("cba", "abc", 2), 0);

    HE_EXPECT_EQ(String::CompareN("abc", "abc", 1), 0);
    HE_EXPECT_EQ(String::CompareN("abc", "acb", 1), 0);
    HE_EXPECT_GT(String::CompareN("cba", "abc", 1), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_CompareI)
{
    HE_EXPECT_EQ(String::CompareI("abc", "ABC"), 0);
    HE_EXPECT_LT(String::CompareI("abc", "ACB"), 0);
    HE_EXPECT_GT(String::CompareI("cba", "ABC"), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_CompareNI)
{
    HE_EXPECT_EQ(String::CompareNI("abc", "ABC", 3), 0);
    HE_EXPECT_LT(String::CompareNI("abc", "ACB", 3), 0);
    HE_EXPECT_GT(String::CompareNI("cba", "ABC", 3), 0);

    HE_EXPECT_EQ(String::CompareNI("abc", "ABC", 2), 0);
    HE_EXPECT_LT(String::CompareNI("abc", "ACB", 2), 0);
    HE_EXPECT_GT(String::CompareNI("cba", "ABC", 2), 0);

    HE_EXPECT_EQ(String::CompareNI("abc", "ABC", 1), 0);
    HE_EXPECT_EQ(String::CompareNI("abc", "ACB", 1), 0);
    HE_EXPECT_GT(String::CompareNI("cba", "ABC", 1), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_Duplicate)
{
    const char* test = "ABC123def";
    char* dup = String::Duplicate(test);

    HE_EXPECT_NE_PTR(dup, test);
    HE_EXPECT_EQ_STR(dup, test);

    Allocator::GetDefault().Free(dup);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_DuplicateN)
{
    const char* test = "ABC123def";
    char* dup = String::DuplicateN(test, 5);

    HE_EXPECT_NE_PTR(dup, test);
    HE_EXPECT_EQ_STR(dup, "ABC12");

    Allocator::GetDefault().Free(dup);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_Equal)
{
    HE_EXPECT(String::Equal("abc", "abc"));
    HE_EXPECT(String::Equal("cba", "cba"));
    HE_EXPECT(!String::Equal("abc", "acb"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_EqualN)
{
    HE_EXPECT(String::EqualN("abc", "abc", 3));
    HE_EXPECT(String::EqualN("cba", "cba", 3));
    HE_EXPECT(!String::EqualN("abc", "acb", 3));

    HE_EXPECT(String::EqualN("abc", "abc", 2));
    HE_EXPECT(String::EqualN("cba", "cba", 2));
    HE_EXPECT(!String::EqualN("abc", "acb", 2));

    HE_EXPECT(String::EqualN("abc", "abc", 1));
    HE_EXPECT(String::EqualN("cba", "cba", 1));
    HE_EXPECT(String::EqualN("abc", "acb", 1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_EqualI)
{
    HE_EXPECT(String::EqualI("abc", "ABC"));
    HE_EXPECT(String::EqualI("cba", "CBA"));
    HE_EXPECT(!String::EqualI("abc", "ACB"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_EqualNI)
{
    HE_EXPECT(String::EqualNI("abc", "ABC", 3));
    HE_EXPECT(String::EqualNI("cba", "CBA", 3));
    HE_EXPECT(!String::EqualNI("abc", "ACB", 3));

    HE_EXPECT(String::EqualNI("abc", "ABC", 2));
    HE_EXPECT(String::EqualNI("cba", "CBA", 2));
    HE_EXPECT(!String::EqualNI("abc", "ACB", 2));

    HE_EXPECT(String::EqualNI("abc", "ABC", 1));
    HE_EXPECT(String::EqualNI("cba", "CBA", 1));
    HE_EXPECT(String::EqualNI("abc", "ACB", 1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_Less)
{
    HE_EXPECT(!String::Less("abc", "abc"));
    HE_EXPECT(!String::Less("cba", "cba"));
    HE_EXPECT(String::Less("abc", "acb"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_LessN)
{
    HE_EXPECT(!String::LessN("abc", "abc", 3));
    HE_EXPECT(!String::LessN("cba", "cba", 3));
    HE_EXPECT(String::LessN("abc", "acb", 3));

    HE_EXPECT(!String::LessN("abc", "abc", 2));
    HE_EXPECT(!String::LessN("cba", "cba", 2));
    HE_EXPECT(String::LessN("abc", "acb", 2));

    HE_EXPECT(!String::LessN("abc", "abc", 1));
    HE_EXPECT(!String::LessN("cba", "cba", 1));
    HE_EXPECT(!String::LessN("abc", "acb", 1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_Copy)
{
    char buf[]{ 'z', 'z', 'z', 'z' };

    {
        const uint32_t srcLen = String::Copy(buf, "abc");
        HE_EXPECT_EQ(srcLen, 3);
        HE_EXPECT_EQ_STR(buf, "abc");
    }

    {
        const uint32_t srcLen = String::Copy(buf, "");
        HE_EXPECT_EQ(srcLen, 0);
        HE_EXPECT_EQ_STR(buf, "");
    }

    {
        const uint32_t srcLen = String::Copy(buf, "abcdefghijklmnopqrstuvwxyz");
        HE_EXPECT_EQ(srcLen, 3);
        HE_EXPECT_EQ_STR(buf, "abc");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_CopyN)
{
    char buf[]{ 'z', 'z', 'z', 'z' };

    {
        const uint32_t srcLen = String::CopyN(buf, "abc", 3);
        HE_EXPECT_EQ(srcLen, 3);
        HE_EXPECT_EQ_STR(buf, "abc");
    }

    {
        const uint32_t srcLen = String::CopyN(buf, "", 0);
        HE_EXPECT_EQ(srcLen, 0);
        HE_EXPECT_EQ_STR(buf, "");
    }

    {
        const uint32_t srcLen = String::CopyN(buf, "abcdefghijklmnopqrstuvwxyz", 2);
        HE_EXPECT_EQ(srcLen, 2);
        HE_EXPECT_EQ_STR(buf, "ab");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_Cat)
{
    char buf[]{ 'a', '\0', 'b', 'b', 'b', 'b', '\0' };

    uint32_t len = String::Cat(buf, "bc");
    HE_EXPECT_EQ(len, 3);
    HE_EXPECT_EQ_STR(buf, "abc");

    len = String::Cat(buf, "xyz");
    HE_EXPECT_EQ(len, 6);
    HE_EXPECT_EQ_STR(buf, "abcxyz");

    len = String::Cat(buf, "123");
    HE_EXPECT_EQ(len, 6);
    HE_EXPECT_EQ_STR(buf, "abcxyz");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_CatN)
{
    char buf[]{ 'a', '\0', 'b', 'b', 'b', 'b', '\0' };

    uint32_t len = String::CatN(buf, "bc", 1);
    HE_EXPECT_EQ(len, 2);
    HE_EXPECT_EQ_STR(buf, "ab");

    len = String::CatN(buf, "xyz", 2);
    HE_EXPECT_EQ(len, 4);
    HE_EXPECT_EQ_STR(buf, "abxy");

    len = String::CatN(buf, "12345", 4);
    HE_EXPECT_EQ(len, 6);
    HE_EXPECT_EQ_STR(buf, "abxy12");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_Find)
{
    constexpr char Haystack[] = "Hello, world!";

    const char* result = String::Find(Haystack, 'z');
    HE_EXPECT(result == nullptr);

    result = String::Find(Haystack, 'o');
    HE_EXPECT(result == (Haystack + 4));

    result = String::Find(Haystack, '!');
    HE_EXPECT(result == (Haystack + 12));

    result = String::Find(Haystack, "Hello!");
    HE_EXPECT(result == nullptr);

    result = String::Find(Haystack, "world");
    HE_EXPECT(result == (Haystack + 7));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_FindN)
{
    constexpr char Haystack[] = "Hello, world!";
    constexpr uint32_t HaystackLen = HE_LENGTH_OF(Haystack) - 1;

    const char* result = String::FindN(Haystack, HaystackLen, 'z');
    HE_EXPECT(result == nullptr);

    result = String::FindN(Haystack, 5, 'o');
    HE_EXPECT(result == (Haystack + 4));

    result = String::FindN(Haystack, 7, '!');
    HE_EXPECT(result == nullptr);

    result = String::FindN(Haystack, HaystackLen, "Hello!");
    HE_EXPECT(result == nullptr);

    result = String::FindN(Haystack, HaystackLen, "world");
    HE_EXPECT(result == (Haystack + 7));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_ToInteger)
{
    HE_EXPECT_EQ(String::ToInteger<char>(""), 0);
    HE_EXPECT_EQ(String::ToInteger<short>(""), 0);
    HE_EXPECT_EQ(String::ToInteger<int>(""), 0);
    HE_EXPECT_EQ(String::ToInteger<long>(""), 0);
    HE_EXPECT_EQ(String::ToInteger<long long>(""), 0);

    HE_EXPECT_EQ(String::ToInteger<unsigned char>(""), 0);
    HE_EXPECT_EQ(String::ToInteger<unsigned short>(""), 0);
    HE_EXPECT_EQ(String::ToInteger<unsigned int>(""), 0);
    HE_EXPECT_EQ(String::ToInteger<unsigned long>(""), 0);
    HE_EXPECT_EQ(String::ToInteger<unsigned long long>(""), 0);

    HE_EXPECT_EQ(String::ToInteger<char>("123"), 123);
    HE_EXPECT_EQ(String::ToInteger<short>("-12345"), -12345);
    HE_EXPECT_EQ(String::ToInteger<int>("12345678"), 12345678);
    HE_EXPECT_EQ(String::ToInteger<long>("-12345678"), -12345678l);
    HE_EXPECT_EQ(String::ToInteger<long long>("1234567890123456789"), 1234567890123456789ll);

    HE_EXPECT_EQ(String::ToInteger<unsigned char>("123"), 123);
    HE_EXPECT_EQ(String::ToInteger<unsigned short>("12345"), 12345);
    HE_EXPECT_EQ(String::ToInteger<unsigned int>("12345678"), 12345678);
    HE_EXPECT_EQ(String::ToInteger<unsigned long>("1234567890"), 1234567890ul);
    HE_EXPECT_EQ(String::ToInteger<unsigned long long>("12345678901234567890"), 12345678901234567890ull);

    HE_EXPECT_EQ(String::ToInteger<char>("0x12", nullptr, 16), 0x12);
    HE_EXPECT_EQ(String::ToInteger<short>("-0x1234", nullptr, 16), -0x1234);
    HE_EXPECT_EQ(String::ToInteger<int>("12345678", nullptr, 16), 0x12345678);
    HE_EXPECT_EQ(String::ToInteger<long>("-12345678", nullptr, 16), -0x12345678l);
    HE_EXPECT_EQ(String::ToInteger<long long>("1234567890123456", nullptr, 16), 0x1234567890123456ll);

    HE_EXPECT_EQ(String::ToInteger<unsigned char>("0x12", nullptr, 16), 0x12);
    HE_EXPECT_EQ(String::ToInteger<unsigned short>("0x1234", nullptr, 16), 0x1234);
    HE_EXPECT_EQ(String::ToInteger<unsigned int>("12345678", nullptr, 16), 0x12345678);
    HE_EXPECT_EQ(String::ToInteger<unsigned long>("12345678", nullptr, 16), 0x12345678ul);
    HE_EXPECT_EQ(String::ToInteger<unsigned long long>("1234567890123456", nullptr, 16), 0x1234567890123456ull);

}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Static_ToFloat)
{
    HE_EXPECT_EQ(String::ToFloat(""), 0.0f);
    HE_EXPECT_EQ(String::ToFloat<double>(""), 0.0);

    HE_EXPECT_EQ(String::ToFloat("123.45"), 123.45f);
    HE_EXPECT_EQ(String::ToFloat<double>("12345.6789"), 12345.6789);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, Constants)
{
    // Changing these are potentially breaking so checking them here so a change is made with thoughtfulness.
    static_assert(std::is_same_v<String::ElementType, char>);
    static_assert(String::MaxEmbedCharacters == 63);
    static_assert(String::MaxHeapCharacters == 0xfffffffe);
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
HE_TEST(core, string, CompareTo)
{
    const String a("Hello, world!");
    const String b("Hello, world!");
    const String c("Goodbye, world!");
    const StringView v("Another one");

    HE_EXPECT_EQ(a.CompareTo(a), 0);
    HE_EXPECT_EQ(a.CompareTo(b), 0);
    HE_EXPECT_GT(a.CompareTo(c), 0);

    HE_EXPECT_EQ(b.CompareTo(a), 0);
    HE_EXPECT_EQ(b.CompareTo(b), 0);
    HE_EXPECT_GT(b.CompareTo(c), 0);

    HE_EXPECT_LT(c.CompareTo(a), 0);
    HE_EXPECT_LT(c.CompareTo(b), 0);
    HE_EXPECT_EQ(c.CompareTo(c), 0);

    HE_EXPECT_GT(c.CompareTo(v), 0);
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
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string, fmt)
{
    constexpr char TestString[] = "Hello, world!";

    const String s1(TestString);

    String s2;
    fmt::format_to(Appender(s2), "{}", s1);
    HE_EXPECT_EQ(s1, s2);
}
