// Copyright Chad Engler

#include "he/core/string_ops.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrEmpty)
{
    static_assert(StrEmpty(nullptr));
    static_assert(StrEmpty(""));
    static_assert(!StrEmpty("abc"));

    HE_EXPECT(StrEmpty(nullptr));
    HE_EXPECT(StrEmpty(""));
    HE_EXPECT(!StrEmpty("abc"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrLenConst)
{
    static_assert(StrLenConst("") == 0);
    static_assert(StrLenConst("abc") == 3);

    HE_EXPECT_EQ(StrLenConst(""), 0);
    HE_EXPECT_EQ(StrLenConst("abc"), 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrLen)
{
    HE_EXPECT_EQ(StrLen(""), 0);
    HE_EXPECT_EQ(StrLen("abc"), 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrLenN)
{
    HE_EXPECT_EQ(StrLenN("", 0), 0);
    HE_EXPECT_EQ(StrLenN("", 1), 0);
    HE_EXPECT_EQ(StrLenN("abc", 3), 3);
    HE_EXPECT_EQ(StrLenN("abc", 1), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrComp)
{
    HE_EXPECT_EQ(StrComp("abc", "abc"), 0);
    HE_EXPECT_LT(StrComp("abc", "acb"), 0);
    HE_EXPECT_GT(StrComp("cba", "abc"), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrCompN)
{
    HE_EXPECT_EQ(StrCompN("abc", "abc", 3), 0);
    HE_EXPECT_LT(StrCompN("abc", "acb", 3), 0);
    HE_EXPECT_GT(StrCompN("cba", "abc", 3), 0);

    HE_EXPECT_EQ(StrCompN("abc", "abc", 2), 0);
    HE_EXPECT_LT(StrCompN("abc", "acb", 2), 0);
    HE_EXPECT_GT(StrCompN("cba", "abc", 2), 0);

    HE_EXPECT_EQ(StrCompN("abc", "abc", 1), 0);
    HE_EXPECT_EQ(StrCompN("abc", "acb", 1), 0);
    HE_EXPECT_GT(StrCompN("cba", "abc", 1), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrCompI)
{
    HE_EXPECT_EQ(StrCompI("abc", "ABC"), 0);
    HE_EXPECT_LT(StrCompI("abc", "ACB"), 0);
    HE_EXPECT_GT(StrCompI("cba", "ABC"), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrCompNI)
{
    HE_EXPECT_EQ(StrCompNI("abc", "ABC", 3), 0);
    HE_EXPECT_LT(StrCompNI("abc", "ACB", 3), 0);
    HE_EXPECT_GT(StrCompNI("cba", "ABC", 3), 0);

    HE_EXPECT_EQ(StrCompNI("abc", "ABC", 2), 0);
    HE_EXPECT_LT(StrCompNI("abc", "ACB", 2), 0);
    HE_EXPECT_GT(StrCompNI("cba", "ABC", 2), 0);

    HE_EXPECT_EQ(StrCompNI("abc", "ABC", 1), 0);
    HE_EXPECT_EQ(StrCompNI("abc", "ACB", 1), 0);
    HE_EXPECT_GT(StrCompNI("cba", "ABC", 1), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrDup)
{
    const char* test = "ABC123def";
    char* dup = StrDup(test);

    HE_EXPECT_NE_PTR(dup, test);
    HE_EXPECT_EQ_STR(dup, test);

    Allocator::GetDefault().Free(dup);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrDupN)
{
    const char* test = "ABC123def";
    char* dup = StrDupN(test, 5);

    HE_EXPECT_NE_PTR(dup, test);
    HE_EXPECT_EQ_STR(dup, "ABC12");

    Allocator::GetDefault().Free(dup);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrEqual)
{
    HE_EXPECT(StrEqual("abc", "abc"));
    HE_EXPECT(StrEqual("cba", "cba"));
    HE_EXPECT(!StrEqual("abc", "acb"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrEqualN)
{
    HE_EXPECT(StrEqualN("abc", "abc", 3));
    HE_EXPECT(StrEqualN("cba", "cba", 3));
    HE_EXPECT(!StrEqualN("abc", "acb", 3));

    HE_EXPECT(StrEqualN("abc", "abc", 2));
    HE_EXPECT(StrEqualN("cba", "cba", 2));
    HE_EXPECT(!StrEqualN("abc", "acb", 2));

    HE_EXPECT(StrEqualN("abc", "abc", 1));
    HE_EXPECT(StrEqualN("cba", "cba", 1));
    HE_EXPECT(StrEqualN("abc", "acb", 1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrEqualI)
{
    HE_EXPECT(StrEqualI("abc", "ABC"));
    HE_EXPECT(StrEqualI("cba", "CBA"));
    HE_EXPECT(!StrEqualI("abc", "ACB"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrEqualNI)
{
    HE_EXPECT(StrEqualNI("abc", "ABC", 3));
    HE_EXPECT(StrEqualNI("cba", "CBA", 3));
    HE_EXPECT(!StrEqualNI("abc", "ACB", 3));

    HE_EXPECT(StrEqualNI("abc", "ABC", 2));
    HE_EXPECT(StrEqualNI("cba", "CBA", 2));
    HE_EXPECT(!StrEqualNI("abc", "ACB", 2));

    HE_EXPECT(StrEqualNI("abc", "ABC", 1));
    HE_EXPECT(StrEqualNI("cba", "CBA", 1));
    HE_EXPECT(StrEqualNI("abc", "ACB", 1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrLess)
{
    HE_EXPECT(!StrLess("abc", "abc"));
    HE_EXPECT(!StrLess("cba", "cba"));
    HE_EXPECT(StrLess("abc", "acb"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrLessN)
{
    HE_EXPECT(!StrLessN("abc", "abc", 3));
    HE_EXPECT(!StrLessN("cba", "cba", 3));
    HE_EXPECT(StrLessN("abc", "acb", 3));

    HE_EXPECT(!StrLessN("abc", "abc", 2));
    HE_EXPECT(!StrLessN("cba", "cba", 2));
    HE_EXPECT(StrLessN("abc", "acb", 2));

    HE_EXPECT(!StrLessN("abc", "abc", 1));
    HE_EXPECT(!StrLessN("cba", "cba", 1));
    HE_EXPECT(!StrLessN("abc", "acb", 1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrCopy)
{
    char buf[]{ 'z', 'z', 'z', 'z' };

    {
        const uint32_t srcLen = StrCopy(buf, "abc");
        HE_EXPECT_EQ(srcLen, 3);
        HE_EXPECT_EQ_STR(buf, "abc");
    }

    {
        const uint32_t srcLen = StrCopy(buf, "");
        HE_EXPECT_EQ(srcLen, 0);
        HE_EXPECT_EQ_STR(buf, "");
    }

    {
        const uint32_t srcLen = StrCopy(buf, "abcdefghijklmnopqrstuvwxyz");
        HE_EXPECT_EQ(srcLen, 3);
        HE_EXPECT_EQ_STR(buf, "abc");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrCopyN)
{
    char buf[]{ 'z', 'z', 'z', 'z' };

    {
        const uint32_t srcLen = StrCopyN(buf, "abc", 3);
        HE_EXPECT_EQ(srcLen, 3);
        HE_EXPECT_EQ_STR(buf, "abc");
    }

    {
        const uint32_t srcLen = StrCopyN(buf, "", 0);
        HE_EXPECT_EQ(srcLen, 0);
        HE_EXPECT_EQ_STR(buf, "");
    }

    {
        const uint32_t srcLen = StrCopyN(buf, "abcdefghijklmnopqrstuvwxyz", 2);
        HE_EXPECT_EQ(srcLen, 2);
        HE_EXPECT_EQ_STR(buf, "ab");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrCat)
{
    char buf[]{ 'a', '\0', 'b', 'b', 'b', 'b', '\0' };

    uint32_t len = StrCat(buf, "bc");
    HE_EXPECT_EQ(len, 3);
    HE_EXPECT_EQ_STR(buf, "abc");

    len = StrCat(buf, "xyz");
    HE_EXPECT_EQ(len, 6);
    HE_EXPECT_EQ_STR(buf, "abcxyz");

    len = StrCat(buf, "123");
    HE_EXPECT_EQ(len, 6);
    HE_EXPECT_EQ_STR(buf, "abcxyz");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrCatN)
{
    char buf[]{ 'a', '\0', 'b', 'b', 'b', 'b', '\0' };

    uint32_t len = StrCatN(buf, "bc", 1);
    HE_EXPECT_EQ(len, 2);
    HE_EXPECT_EQ_STR(buf, "ab");

    len = StrCatN(buf, "xyz", 2);
    HE_EXPECT_EQ(len, 4);
    HE_EXPECT_EQ_STR(buf, "abxy");

    len = StrCatN(buf, "12345", 4);
    HE_EXPECT_EQ(len, 6);
    HE_EXPECT_EQ_STR(buf, "abxy12");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrFind)
{
    constexpr char Haystack[] = "Hello, world!";

    const char* result = StrFind(Haystack, 'z');
    HE_EXPECT(result == nullptr);

    result = StrFind(Haystack, 'o');
    HE_EXPECT(result == (Haystack + 4));

    result = StrFind(Haystack, '!');
    HE_EXPECT(result == (Haystack + 12));

    result = StrFind(Haystack, "Hello!");
    HE_EXPECT(result == nullptr);

    result = StrFind(Haystack, "world");
    HE_EXPECT(result == (Haystack + 7));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrFindN)
{
    constexpr char Haystack[] = "Hello, world!";
    constexpr uint32_t HaystackLen = HE_LENGTH_OF(Haystack) - 1;

    const char* result = StrFindN(Haystack, HaystackLen, 'z');
    HE_EXPECT(result == nullptr);

    result = StrFindN(Haystack, 5, 'o');
    HE_EXPECT(result == (Haystack + 4));

    result = StrFindN(Haystack, 7, '!');
    HE_EXPECT(result == nullptr);

    result = StrFindN(Haystack, HaystackLen, "Hello!");
    HE_EXPECT(result == nullptr);

    result = StrFindN(Haystack, HaystackLen, "world");
    HE_EXPECT(result == (Haystack + 7));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrFindLast)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrFindLastN)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrToInt)
{
    HE_EXPECT_EQ(StrToInt<char>(""), 0);
    HE_EXPECT_EQ(StrToInt<short>(""), 0);
    HE_EXPECT_EQ(StrToInt<int>(""), 0);
    HE_EXPECT_EQ(StrToInt<long>(""), 0);
    HE_EXPECT_EQ(StrToInt<long long>(""), 0);

    HE_EXPECT_EQ(StrToInt<unsigned char>(""), 0);
    HE_EXPECT_EQ(StrToInt<unsigned short>(""), 0);
    HE_EXPECT_EQ(StrToInt<unsigned int>(""), 0);
    HE_EXPECT_EQ(StrToInt<unsigned long>(""), 0);
    HE_EXPECT_EQ(StrToInt<unsigned long long>(""), 0);

    HE_EXPECT_EQ(StrToInt<char>("123"), 123);
    HE_EXPECT_EQ(StrToInt<short>("-12345"), -12345);
    HE_EXPECT_EQ(StrToInt<int>("12345678"), 12345678);
    HE_EXPECT_EQ(StrToInt<long>("-12345678"), -12345678l);
    HE_EXPECT_EQ(StrToInt<long long>("1234567890123456789"), 1234567890123456789ll);

    HE_EXPECT_EQ(StrToInt<unsigned char>("123"), 123);
    HE_EXPECT_EQ(StrToInt<unsigned short>("12345"), 12345);
    HE_EXPECT_EQ(StrToInt<unsigned int>("12345678"), 12345678);
    HE_EXPECT_EQ(StrToInt<unsigned long>("1234567890"), 1234567890ul);
    HE_EXPECT_EQ(StrToInt<unsigned long long>("12345678901234567890"), 12345678901234567890ull);

    HE_EXPECT_EQ(StrToInt<char>("0x12", nullptr, 16), 0x12);
    HE_EXPECT_EQ(StrToInt<short>("-0x1234", nullptr, 16), -0x1234);
    HE_EXPECT_EQ(StrToInt<int>("12345678", nullptr, 16), 0x12345678);
    HE_EXPECT_EQ(StrToInt<long>("-12345678", nullptr, 16), -0x12345678l);
    HE_EXPECT_EQ(StrToInt<long long>("1234567890123456", nullptr, 16), 0x1234567890123456ll);

    HE_EXPECT_EQ(StrToInt<unsigned char>("0x12", nullptr, 16), 0x12);
    HE_EXPECT_EQ(StrToInt<unsigned short>("0x1234", nullptr, 16), 0x1234);
    HE_EXPECT_EQ(StrToInt<unsigned int>("12345678", nullptr, 16), 0x12345678);
    HE_EXPECT_EQ(StrToInt<unsigned long>("12345678", nullptr, 16), 0x12345678ul);
    HE_EXPECT_EQ(StrToInt<unsigned long long>("1234567890123456", nullptr, 16), 0x1234567890123456ull);

}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_ops, StrToFloat)
{
    HE_EXPECT_EQ(StrToFloat(""), 0.0f);
    HE_EXPECT_EQ(StrToFloat<double>(""), 0.0);

    HE_EXPECT_EQ(StrToFloat("123.45"), 123.45f);
    HE_EXPECT_EQ(StrToFloat<double>("12345.6789"), 12345.6789);
}
