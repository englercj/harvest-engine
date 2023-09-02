// Copyright Chad Engler

#include "he/core/memory_ops.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, MemCopy)
{
    const uint32_t src[]{ 1, 2, 3 };
    uint32_t dst[HE_LENGTH_OF(src)];

    static_assert(sizeof(src) == sizeof(dst));

    void* p = MemCopy(dst, src, sizeof(src));
    HE_EXPECT_EQ(dst[0], src[0]);
    HE_EXPECT_EQ(dst[1], src[1]);
    HE_EXPECT_EQ(dst[2], src[2]);
    HE_EXPECT(p == dst);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, MemMove)
{
    const uint32_t src[]{ 1, 2, 3 };
    uint32_t dst[HE_LENGTH_OF(src)];

    static_assert(sizeof(src) == sizeof(dst));

    void* p = MemMove(dst, src, sizeof(src));
    HE_EXPECT_EQ(dst[0], src[0]);
    HE_EXPECT_EQ(dst[1], src[1]);
    HE_EXPECT_EQ(dst[2], src[2]);
    HE_EXPECT(p == dst);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, MemCmp)
{
    const int a[]{ 1, 2, 3 };
    const int b[]{ 1, 2, 3 };
    const int c[]{ 1, 2, 4 };

    static_assert(sizeof(a) == sizeof(b));
    static_assert(sizeof(a) == sizeof(c));

    HE_EXPECT_EQ(MemCmp(a, b, sizeof(a)), 0);
    HE_EXPECT_LT(MemCmp(a, c, sizeof(a)), 0);
    HE_EXPECT_EQ(MemCmp(b, a, sizeof(a)), 0);
    HE_EXPECT_LT(MemCmp(b, c, sizeof(a)), 0);
    HE_EXPECT_GT(MemCmp(c, a, sizeof(a)), 0);
    HE_EXPECT_GT(MemCmp(c, b, sizeof(a)), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, MemSet)
{
    uint8_t a[10]{};

    void* p = MemSet(a, 0x02, sizeof(a));
    HE_EXPECT_EQ(a[0], 2);
    HE_EXPECT_EQ(a[5], 2);
    HE_EXPECT_EQ(a[9], 2);
    HE_EXPECT(p == a);

    MemSet(a, 0x01, 2);
    HE_EXPECT_EQ(a[0], 1);
    HE_EXPECT_EQ(a[1], 1);
    HE_EXPECT_EQ(a[2], 2);
    HE_EXPECT_EQ(a[3], 2);
    HE_EXPECT_EQ(a[5], 2);
    HE_EXPECT_EQ(a[9], 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, MemChr)
{
    constexpr char Fixture[] = "Hello, world!";

    HE_EXPECT_EQ(MemChr(Fixture, 0, sizeof(Fixture)), Fixture + 13);
    HE_EXPECT_EQ(MemChr(Fixture, 0x61, sizeof(Fixture)), nullptr);
    HE_EXPECT_EQ(MemChr(Fixture, 0x63, sizeof(Fixture)), nullptr);
    HE_EXPECT_EQ(MemChr(Fixture, 0x65, sizeof(Fixture)), Fixture + 1);
    HE_EXPECT_EQ(MemChr(Fixture, 'o', sizeof(Fixture)), Fixture + 4);
    HE_EXPECT_EQ(MemChr(Fixture, 'Z', sizeof(Fixture)), nullptr);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, MemZero)
{
    uint8_t a[10]{};
    uint8_t zeroes[HE_LENGTH_OF(a)]{};

    static_assert(sizeof(a) == sizeof(zeroes));

    for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
    {
        HE_EXPECT_EQ(a[i], zeroes[i]);
    }

    MemSet(a, 0x02, sizeof(a));
    for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
    {
        HE_EXPECT_NE(a[i], zeroes[i]);
    }

    void* p = MemZero(a, sizeof(a));
    HE_EXPECT(p == a);
    for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
    {
        HE_EXPECT_EQ(a[i], zeroes[i]);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, MemEqual)
{
    uint32_t a[16];
    uint32_t zeroes[HE_LENGTH_OF(a)]{};

    static_assert(sizeof(a) == sizeof(zeroes));

    MemSet(a, 0x02, sizeof(a));

    HE_EXPECT(!MemEqual(a, zeroes, sizeof(a)));
    HE_EXPECT(MemEqual(a, a, sizeof(a)));

    MemZero(a, sizeof(a));
    HE_EXPECT(MemEqual(a, zeroes, sizeof(a)));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, MemLess)
{
    uint32_t a[16];
    uint32_t zeroes[HE_LENGTH_OF(a)]{};

    static_assert(sizeof(a) == sizeof(zeroes));

    MemSet(a, 0x02, sizeof(a));

    HE_EXPECT(!MemLess(a, zeroes, sizeof(a)));
    HE_EXPECT(!MemLess(a, a, sizeof(a)));
    HE_EXPECT(MemLess(zeroes, a, sizeof(a)));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, ByteSwap)
{
    HE_EXPECT_EQ(ByteSwap(uint16_t(0x0102)), uint16_t(0x0201));
    HE_EXPECT_EQ(ByteSwap(uint32_t(0x01020304)), uint32_t(0x04030201));
    HE_EXPECT_EQ(ByteSwap(uint64_t(0x0102030405060708)), uint64_t(0x0807060504030201));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, Rotl32)
{
    HE_EXPECT_EQ(Rotl32(0x00000001,  1), 0x00000002);
    HE_EXPECT_EQ(Rotl32(0x00000001,  4), 0x00000010);
    HE_EXPECT_EQ(Rotl32(0x00000001,  8), 0x00000100);
    HE_EXPECT_EQ(Rotl32(0x00000001, 16), 0x00010000);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, Rotl64)
{
    HE_EXPECT_EQ(Rotl64(0x0000000000000001,  1), 0x0000000000000002);
    HE_EXPECT_EQ(Rotl64(0x0000000000000001,  4), 0x0000000000000010);
    HE_EXPECT_EQ(Rotl64(0x0000000000000001,  8), 0x0000000000000100);
    HE_EXPECT_EQ(Rotl64(0x0000000000000001, 16), 0x0000000000010000);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, Rotr32)
{
    HE_EXPECT_EQ(Rotr32(0x00000001,  1), 0x80000000);
    HE_EXPECT_EQ(Rotr32(0x00000001,  4), 0x10000000);
    HE_EXPECT_EQ(Rotr32(0x00000001,  8), 0x01000000);
    HE_EXPECT_EQ(Rotr32(0x00000001, 16), 0x00010000);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, Rotr64)
{
    HE_EXPECT_EQ(Rotr64(0x0000000000000001,  1), 0x8000000000000000);
    HE_EXPECT_EQ(Rotr64(0x0000000000000001,  4), 0x1000000000000000);
    HE_EXPECT_EQ(Rotr64(0x0000000000000001,  8), 0x0100000000000000);
    HE_EXPECT_EQ(Rotr64(0x0000000000000001, 16), 0x0001000000000000);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, LoadLE)
{
    const uint16_t v16 = 0x0102;
    HE_EXPECT_EQ(LoadLE(v16), 0x0102);

    const uint32_t v32 = 0x01020304;
    HE_EXPECT_EQ(LoadLE(v32), 0x01020304);

    const uint64_t v64 = 0x0102030405060708;
    HE_EXPECT_EQ(LoadLE(v64), 0x0102030405060708);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, LoadBE)
{
    const uint16_t v16 = 0x0102;
    HE_EXPECT_EQ(LoadBE(v16), 0x0201);

    const uint32_t v32 = 0x01020304;
    HE_EXPECT_EQ(LoadBE(v32), 0x04030201);

    const uint64_t v64 = 0x0102030405060708;
    HE_EXPECT_EQ(LoadBE(v64), 0x0807060504030201);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, StoreLE)
{
    uint16_t v16 = 0;
    StoreLE(v16, 0x0102);
    HE_EXPECT_EQ(v16, 0x0102);

    uint32_t v32 = 0;
    StoreLE(v32, 0x01020304);
    HE_EXPECT_EQ(v32, 0x01020304);

    uint64_t v64 = 0x0102030405060708;
    StoreLE(v64, 0x0102030405060708);
    HE_EXPECT_EQ(v64, 0x0102030405060708);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, memory_ops, StoreBE)
{
    uint16_t v16 = 0;
    StoreBE(v16, 0x0102);
    HE_EXPECT_EQ(v16, 0x0201);

    uint32_t v32 = 0;
    StoreBE(v32, 0x01020304);
    HE_EXPECT_EQ(v32, 0x04030201);

    uint64_t v64 = 0x0102030405060708;
    StoreBE(v64, 0x0102030405060708);
    HE_EXPECT_EQ(v64, 0x0807060504030201);
}
