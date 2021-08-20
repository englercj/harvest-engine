// Copyright Chad Engler

#include "he/core/hash.h"

#include "he/core/test.h"

#include <random>

using namespace he;

// ------------------------------------------------------------------------------------------------
template <typename H>
static void TestHashBool(bool value)
{
    H a, b;
    typename H::ValueType v0, v1;

    a.Scalar(value);
    v0 = a.Done();

    uint8_t expected = value ? 1 : 0;
    b.Data(&expected, sizeof(expected));
    v1 = b.Done();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
template <typename H, typename T>
static void TestHashScalar(T value)
{
    H a, b;
    typename H::ValueType v0, v1;

    a.Scalar(value);
    v0 = a.Done();

    b.Data(&value, sizeof(value));
    v1 = b.Done();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
template <typename H>
static void TestHashString(const char* value)
{
    H a, b;
    typename H::ValueType v0, v1;

    a.String(value);
    v0 = a.Done();

    b.Data(value, String::Length(value));
    v1 = b.Done();

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
    {
        H a;
        typename H::ValueType v0, v1;

        const char* x = "foobar";
        a.String(x);
        v0 = a.Done();

        a.Reset();
        a.String(x);
        v1 = a.Done();

        HE_EXPECT_EQ(v0, v1);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, FNV32)
{
    TestHashAlgorithm<FNV32>();

    HE_EXPECT_EQ(FNV32::HashString(""), 0x811c9dc5);
    HE_EXPECT_EQ(FNV32::HashString("a"), 0xe40c292c);
    HE_EXPECT_EQ(FNV32::HashString("foobar"), 0xbf9cf968);
    HE_EXPECT_EQ(FNV32::HashString("foobarabcdefghiglmnop"), 0xaec8b08c);
    HE_EXPECT_EQ(FNV32::HashString("\200"), 0x850b939f);
    HE_EXPECT_EQ(FNV32::HashData("foobar", 6), 0xbf9cf968);
    HE_EXPECT_EQ(FNV32::HashScalar('\200'), 0x850b939f);

    static_assert(FNV32::HashString("foobar") == 0xbf9cf968, "");
    static_assert(FNV32::HashString("foobarabcdefghiglmnop") == 0xaec8b08c, "");
    static_assert(FNV32::HashString("\200") == 0x850b939f, "");
    static_assert(FNV32::HashStringN("apple", 1) == 0xe40c292c, "");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, FNV64)
{
    TestHashAlgorithm<FNV64>();

    HE_EXPECT_EQ(FNV64::HashString(""), 0xcbf29ce484222325);
    HE_EXPECT_EQ(FNV64::HashString("a"), 0xaf63dc4c8601ec8c);
    HE_EXPECT_EQ(FNV64::HashString("foobar"), 0x85944171f73967e8);
    HE_EXPECT_EQ(FNV64::HashString("foobarabcdefghiglmnop"), 0x9c6beb459f40f26c);
    HE_EXPECT_EQ(FNV64::HashString("\200"), 0xaf643d4c8602915f);
    HE_EXPECT_EQ(FNV64::HashData("foobar", 6), 0x85944171f73967e8);
    HE_EXPECT_EQ(FNV64::HashScalar('\200'), 0xaf643d4c8602915f);

    static_assert(FNV64::HashString("foobar") == 0x85944171f73967e8, "");
    static_assert(FNV64::HashString("foobarabcdefghiglmnop") == 0x9c6beb459f40f26c, "");
    static_assert(FNV64::HashString("\200") == 0xaf643d4c8602915f, "");
    static_assert(FNV64::HashStringN("apple", 1) == 0xaf63dc4c8601ec8c, "");
}
