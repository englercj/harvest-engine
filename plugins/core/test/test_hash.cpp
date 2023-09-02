// Copyright Chad Engler

#include "he/core/hash.h"
#include "he/core/hash_fmt.h"

#include "he/core/string_ops.h"
#include "he/core/random.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
template <typename H>
static void TestHashBool(bool value)
{
    Hash<H> a, b;
    typename H::ValueType v0, v1;

    a.Update(value);
    v0 = a.Final();

    uint8_t expected = value ? 1 : 0;
    b.Update(&expected, sizeof(expected));
    v1 = b.Final();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
template <typename H, typename T>
static void TestHashScalar(T value)
{
    Hash<H> a, b;
    typename H::ValueType v0, v1;

    a.Update(value);
    v0 = a.Final();

    b.Update(&value, sizeof(value));
    v1 = b.Final();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
template <typename H>
static void TestHashString(const char* value)
{
    Hash<H> a, b;
    typename H::ValueType v0, v1;

    a.Update(value);
    v0 = a.Final();

    b.Update(value, StrLen(value));
    v1 = b.Final();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
template <typename H>
static void TestHashAlgorithm()
{
    TestHashBool<H>(true);
    TestHashBool<H>(false);

    TestHashScalar<H, bool>(true);
    TestHashScalar<H, bool>(false);

    TestHashScalar<H, int8_t>(0x12);
    TestHashScalar<H, int16_t>(0x1234);
    TestHashScalar<H, int32_t>(0x12345678);
    TestHashScalar<H, int64_t>(0x12345678901234);

    TestHashScalar<H, uint8_t>(0x12);
    TestHashScalar<H, uint16_t>(0x1234);
    TestHashScalar<H, uint32_t>(0x12345678);
    TestHashScalar<H, uint64_t>(0x12345678901234);

    TestHashString<H>("foobar");
    TestHashString<H>("foobarabcdefghiglmnop");

    // Reset allows for reuse
    Hash<H> a;
    typename H::ValueType v0, v1;

    const char* x = "foobar";
    a.Update(x);
    v0 = a.Final();

    a.Reset();

    a.Update(x);
    v1 = a.Final();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, CombineHash)
{
    {
        uint16_t a = 0x1234;
        uint16_t b = 0x5678;
        uint16_t c = CombineHash16(a, b);
        HE_EXPECT_NE(a, c);
        HE_EXPECT_NE(b, c);
    }

    {
        uint32_t a = 0x12345678;
        uint32_t b = 0x87654321;
        uint32_t c = CombineHash32(a, b);
        HE_EXPECT_NE(a, c);
        HE_EXPECT_NE(b, c);
    }

    {
        uint64_t a = 0x1234567890123456;
        uint64_t b = 0x8765432187654321;
        uint64_t c = CombineHash64(a, b);
        HE_EXPECT_NE(a, c);
        HE_EXPECT_NE(b, c);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, Mix)
{
    {
        uint32_t a = 0x12345678;
        uint32_t b = Mix32(a);
        HE_EXPECT_NE(a, b);
    }

    {
        uint64_t a = 0x1234567890123456;
        uint64_t b = Mix64(a);
        HE_EXPECT_NE(a, b);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, FNV32)
{
    TestHashAlgorithm<FNV32>();

    HE_EXPECT_EQ(FNV32::String(""), 0x811c9dc5);
    HE_EXPECT_EQ(FNV32::String("a"), 0xe40c292c);
    HE_EXPECT_EQ(FNV32::String("foobar"), 0xbf9cf968);
    HE_EXPECT_EQ(FNV32::String("foobarabcdefghiglmnop"), 0xaec8b08c);
    HE_EXPECT_EQ(FNV32::String("\200"), 0x850b939f);

    HE_EXPECT_EQ(FNV32::Mem("foobar", 6), 0xbf9cf968);
    HE_EXPECT_EQ(FNV32::Mem("\200", 1), 0x850b939f);

    static_assert(FNV32::String("foobar") == 0xbf9cf968, "");
    static_assert(FNV32::String("foobarabcdefghiglmnop") == 0xaec8b08c, "");
    static_assert(FNV32::String("\200") == 0x850b939f, "");
    static_assert(FNV32::String({ "apple", 1 }) == 0xe40c292c, "");

    uint32_t h = FNV32::String("abc");
    h = FNV32::String("def", h);
    HE_EXPECT_EQ(h, FNV32::String("abcdef"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, FNV64)
{
    TestHashAlgorithm<FNV64>();

    HE_EXPECT_EQ(FNV64::String(""), 0xcbf29ce484222325);
    HE_EXPECT_EQ(FNV64::String("a"), 0xaf63dc4c8601ec8c);
    HE_EXPECT_EQ(FNV64::String("foobar"), 0x85944171f73967e8);
    HE_EXPECT_EQ(FNV64::String("foobarabcdefghiglmnop"), 0x9c6beb459f40f26c);
    HE_EXPECT_EQ(FNV64::String("\200"), 0xaf643d4c8602915f);

    HE_EXPECT_EQ(FNV64::Mem("foobar", 6), 0x85944171f73967e8);
    HE_EXPECT_EQ(FNV64::Mem("\200", 1), 0xaf643d4c8602915f);

    static_assert(FNV64::String("foobar") == 0x85944171f73967e8, "");
    static_assert(FNV64::String("foobarabcdefghiglmnop") == 0x9c6beb459f40f26c, "");
    static_assert(FNV64::String("\200") == 0xaf643d4c8602915f, "");
    static_assert(FNV64::String({ "apple", 1 }) == 0xaf63dc4c8601ec8c, "");

    uint64_t h = FNV64::String("abc");
    h = FNV64::String("def", h);
    HE_EXPECT_EQ(h, FNV64::String("abcdef"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, CRC32C)
{
    TestHashAlgorithm<CRC32C>();

    HE_EXPECT_EQ(CRC32C::Mem("", 0), 0);
    HE_EXPECT_EQ(CRC32C::Mem("a", 1), 0xc1d04330);
    HE_EXPECT_EQ(CRC32C::Mem("foobar", 6), 0x0d5f5c7f);
    HE_EXPECT_EQ(CRC32C::Mem("foobarabcdefghiglmnop", 21), 0xfef874d9);
    HE_EXPECT_EQ(CRC32C::Mem("\200", 1), 0xd08b6829);

    uint32_t h = CRC32C::Mem("abc", 3);
    h = CRC32C::Mem("def", 3, h);
    HE_EXPECT_EQ(h, CRC32C::Mem("abcdef", 6));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, WyHash)
{
    TestHashAlgorithm<WyHash>();

    HE_EXPECT_EQ(WyHash::Mem("", 0), 0x0409638ee2bde459);
    HE_EXPECT_EQ(WyHash::Mem("a", 1), 0x28d2053309d28531);
    HE_EXPECT_EQ(WyHash::Mem("foobar", 6), 0x0b9d35b96e1f6fe2);
    HE_EXPECT_EQ(WyHash::Mem("foobarabcdefghiglmnop", 21), 0x698f1bc80e749f46);
    HE_EXPECT_EQ(WyHash::Mem("\200", 1), 0x26f0dff3757426db);

    HE_EXPECT_EQ(WyHash::Mem("foobar", 6), 0x0b9d35b96e1f6fe2);
    HE_EXPECT_EQ(WyHash::Mem("\200", 1), 0x26f0dff3757426db);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, MD5)
{
    TestHashAlgorithm<MD5>();

    struct TestCase
    {
        const char* s;
        uint8_t hash[16];
    } tests[] =
    {
        {
            "",
            { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e },
        },
        {
            "The quick brown fox jumps over the lazy dog",
            { 0x9e, 0x10, 0x7d, 0x9d, 0x37, 0x2b, 0xb6, 0x82, 0x6b, 0xd8, 0x1d, 0x35, 0x42, 0xa4, 0x19, 0xd6 },
        },
        {
            "abc",
            { 0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0, 0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72 },
        },
        {
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            { 0x82, 0x15, 0xef, 0x07, 0x96, 0xa2, 0x0b, 0xca, 0xaa, 0xe1, 0x16, 0xd3, 0x87, 0x6c, 0x66, 0x4a },
        },
    };

    for (const TestCase& tc : tests)
    {
        const MD5::Value value = MD5::Mem(tc.s, StrLen(tc.s));
        static_assert(sizeof(value.bytes) == sizeof(tc.hash));
        HE_EXPECT_EQ_MEM(value.bytes, tc.hash, sizeof(value.bytes));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, SHA1)
{
    TestHashAlgorithm<SHA1>();

    struct TestCase
    {
        const char* s;
        uint8_t hash[20];
    } tests[] =
    {
        {
            "",
            { 0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09 },
        },
        {
            "The quick brown fox jumps over the lazy dog",
            { 0x2f, 0xd4, 0xe1, 0xc6, 0x7a, 0x2d, 0x28, 0xfc, 0xed, 0x84, 0x9e, 0xe1, 0xbb, 0x76, 0xe7, 0x39, 0x1b, 0x93, 0xeb, 0x12 },
        },
        {
            "abc",
            { 0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d },
        },
        {
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            { 0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae, 0x4a, 0xa1, 0xf9, 0x51, 0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1 },
        },
    };

    for (const TestCase& tc : tests)
    {
        const SHA1::Value value = SHA1::Mem(tc.s, StrLen(tc.s));
        static_assert(sizeof(value.bytes) == sizeof(tc.hash));
        HE_EXPECT_EQ_MEM(value.bytes, tc.hash, sizeof(value.bytes));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, SHA256)
{
    TestHashAlgorithm<SHA256>();

    struct TestCase
    {
        const char* s;
        uint8_t hash[32];
    } tests[] =
    {
        {
            "",
            { 0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55 },
        },
        {
            "The quick brown fox jumps over the lazy dog",
            { 0xd7, 0xa8, 0xfb, 0xb3, 0x07, 0xd7, 0x80, 0x94, 0x69, 0xca, 0x9a, 0xbc, 0xb0, 0x08, 0x2e, 0x4f, 0x8d, 0x56, 0x51, 0xe4, 0x6d, 0x3c, 0xdb, 0x76, 0x2d, 0x02, 0xd0, 0xbf, 0x37, 0xc9, 0xe5, 0x92 },
        },
        {
            "abc",
            { 0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad },
        },
        {
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            { 0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8, 0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39, 0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67, 0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1 },
        },
    };

    for (const TestCase& tc : tests)
    {
        const SHA256::Value value = SHA256::Mem(tc.s, StrLen(tc.s));
        static_assert(sizeof(value.bytes) == sizeof(tc.hash));
        HE_EXPECT_EQ_MEM(value.bytes, tc.hash, sizeof(value.bytes));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, Hasher)
{
    enum TestEnum { V = 123 };
    enum class TestEnumClass { V = 123 };

    static_assert(Hasher<bool>()(true) == 0xffffffffffffffff);
    static_assert(Hasher<bool>()(false) == 0);
    static_assert(Hasher<char>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned char>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<signed char>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<wchar_t>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<char8_t>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<char16_t>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<char32_t>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<short>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned short>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<int>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned int>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<long>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned long>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<long long>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned long long>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<TestEnum>()(TestEnum::V) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<TestEnumClass>()(TestEnumClass::V) == 0x7fcb4990c961f6a8);

    HE_EXPECT_EQ(Hasher<bool>()(true), 0xffffffffffffffff);
    HE_EXPECT_EQ(Hasher<bool>()(false), 0);
    HE_EXPECT_EQ(Hasher<char>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned char>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<signed char>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<wchar_t>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<char8_t>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<char16_t>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<char32_t>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<short>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned short>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<int>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned int>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<long>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned long>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<long long>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned long long>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<TestEnum>()(TestEnum::V), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<TestEnumClass>()(TestEnumClass::V), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<void*>()((void*)123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<float>()(123), 0xf8450ab74f865b48);
    HE_EXPECT_EQ(Hasher<double>()(123), 0xfde89695555aaf6f);
    HE_EXPECT_EQ(Hasher<long double>()(123), 0xfde89695555aaf6f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, GetHashCode)
{
    enum TestEnum { V = 123 };
    enum class TestEnumClass { V = 123 };

    static_assert(GetHashCode<bool>(true) == Hasher<bool>()(true));
    static_assert(GetHashCode<bool>(false) == Hasher<bool>()(false));
    static_assert(GetHashCode<char>(123) == Hasher<char>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned char>()(123));
    static_assert(GetHashCode<signed >(123) == Hasher<signed char>()(123));
    static_assert(GetHashCode<wchar_t>(123) == Hasher<wchar_t>()(123));
    static_assert(GetHashCode<char8_t>(123) == Hasher<char8_t>()(123));
    static_assert(GetHashCode<char16_t>(123) == Hasher<char16_t>()(123));
    static_assert(GetHashCode<char32_t>(123) == Hasher<char32_t>()(123));
    static_assert(GetHashCode<short>(123) == Hasher<short>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned short>()(123));
    static_assert(GetHashCode<int>(123) == Hasher<int>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned int>()(123));
    static_assert(GetHashCode<long>(123) == Hasher<long>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned long>()(123));
    static_assert(GetHashCode<long >(123) == Hasher<long long>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned long long>()(123));
    static_assert(GetHashCode<TestEnum>(TestEnum::V) == Hasher<TestEnum>()(TestEnum::V));
    static_assert(GetHashCode<TestEnumClass>(TestEnumClass::V) == Hasher<TestEnumClass>()(TestEnumClass::V));

    HE_EXPECT_EQ(GetHashCode<bool>(true), Hasher<bool>()(true));
    HE_EXPECT_EQ(GetHashCode<bool>(false), Hasher<bool>()(false));
    HE_EXPECT_EQ(GetHashCode<char>(123), Hasher<char>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned char>()(123));
    HE_EXPECT_EQ(GetHashCode<signed >(123), Hasher<signed char>()(123));
    HE_EXPECT_EQ(GetHashCode<wchar_t>(123), Hasher<wchar_t>()(123));
    HE_EXPECT_EQ(GetHashCode<char8_t>(123), Hasher<char8_t>()(123));
    HE_EXPECT_EQ(GetHashCode<char16_t>(123), Hasher<char16_t>()(123));
    HE_EXPECT_EQ(GetHashCode<char32_t>(123), Hasher<char32_t>()(123));
    HE_EXPECT_EQ(GetHashCode<short>(123), Hasher<short>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned short>()(123));
    HE_EXPECT_EQ(GetHashCode<int>(123), Hasher<int>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned int>()(123));
    HE_EXPECT_EQ(GetHashCode<long>(123), Hasher<long>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned long>()(123));
    HE_EXPECT_EQ(GetHashCode<long >(123), Hasher<long long>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned long long>()(123));
    HE_EXPECT_EQ(GetHashCode<TestEnum>(TestEnum::V), Hasher<TestEnum>()(TestEnum::V));
    HE_EXPECT_EQ(GetHashCode<TestEnumClass>(TestEnumClass::V), Hasher<TestEnumClass>()(TestEnumClass::V));
    HE_EXPECT_EQ(GetHashCode<void*>((void*)123), Hasher<void*>()((void*)123));
    HE_EXPECT_EQ(GetHashCode<float>(123), Hasher<float>()(123));
    HE_EXPECT_EQ(GetHashCode<double>(123), Hasher<double>()(123));
    HE_EXPECT_EQ(GetHashCode<long double>(123), Hasher<long double>()(123));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, HasHashCode)
{
    struct Nope1 {};
    struct Nope2 { uint32_t HashCode() const; };
    struct Nope3 { uint64_t HashCode(); };
    struct Nope4 { int64_t HashCode() const; };
    struct Yup1 { uint64_t HashCode() const; };
    struct Yup2 { uint64_t HashCode() const noexcept; };

    static_assert(!HasHashCode<Nope1>);
    static_assert(!HasHashCode<Nope2>);
    static_assert(!HasHashCode<Nope3>);
    static_assert(!HasHashCode<Nope4>);
    static_assert(HasHashCode<Yup1>);
    static_assert(HasHashCode<Yup2>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, Hashable)
{
    enum TestEnum {};
    enum class TestEnumClass {};

    static_assert(Hashable<bool>);
    static_assert(Hashable<char>);
    static_assert(Hashable<unsigned char>);
    static_assert(Hashable<signed char>);
    static_assert(Hashable<wchar_t>);
    static_assert(Hashable<char8_t>);
    static_assert(Hashable<char16_t>);
    static_assert(Hashable<char32_t>);
    static_assert(Hashable<short>);
    static_assert(Hashable<unsigned short>);
    static_assert(Hashable<int>);
    static_assert(Hashable<unsigned int>);
    static_assert(Hashable<long>);
    static_assert(Hashable<unsigned long>);
    static_assert(Hashable<long long>);
    static_assert(Hashable<unsigned long long>);
    static_assert(Hashable<TestEnum>);
    static_assert(Hashable<TestEnumClass>);
    static_assert(Hashable<void*>);
    static_assert(Hashable<float>);
    static_assert(Hashable<double>);
    static_assert(Hashable<long double>);

    struct HashableObj { uint64_t HashCode() const; };
    static_assert(Hashable<HashableObj>);

    struct NotHashableObj {};
    static_assert(!Hashable<NotHashableObj>);
}
