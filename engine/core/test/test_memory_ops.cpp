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
