// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/test.h"
#include "he/core/utils.h"

using namespace he;

static void TestAllocator(Allocator& alloc)
{
    // Malloc -> Realloc -> Free
    {
        void* mem = alloc.Malloc(16);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, Allocator::DefaultAlignment));

        mem = alloc.Realloc(mem, 32);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, Allocator::DefaultAlignment));

        alloc.Free(mem);
    }

    // Malloc overaligned
    {
        void* mem = alloc.Malloc(16, 128);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, 128));

        mem = alloc.Realloc(mem, 32, 128);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, 128));

        alloc.Free(mem);
    }

    // Malloc unique
    {
        void* mem1 = alloc.Malloc(16);
        void* mem2 = alloc.Malloc(16);
        HE_EXPECT(mem1 != mem2);

        alloc.Free(mem1);
        alloc.Free(mem2);
    }

    // Realloc as all 3 operations
    {
        void* mem = alloc.Realloc(nullptr, 16);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, Allocator::DefaultAlignment));

        mem = alloc.Realloc(mem, 32);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, Allocator::DefaultAlignment));

        alloc.Realloc(mem, 0);
    }

    // Malloc<T> trivial
    {
        uint32_t* mem = alloc.Malloc<uint32_t>(16);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, alignof(uint32_t)));

        alloc.Free(mem);
    }

    // Malloc<T> non-trivial
    {
        static uint32_t s_ctorCount = 0;
        static uint32_t s_dtorCount = 0;
        struct CtorTest { CtorTest() { ++s_ctorCount; } ~CtorTest() { ++s_dtorCount; } };

        CtorTest* mem = alloc.Malloc<CtorTest>(16);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, alignof(CtorTest)));
        HE_EXPECT_EQ(s_ctorCount, 0);
        HE_EXPECT_EQ(s_dtorCount, 0);

        alloc.Free(mem);
        HE_EXPECT_EQ(s_ctorCount, 0);
        HE_EXPECT_EQ(s_dtorCount, 0);
    }

    // New -> Delete trivial
    {
        uint32_t* mem = alloc.New<uint32_t>();
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, alignof(uint32_t)));

        alloc.Delete(mem);
    }

    // New -> Delete non-trivial
    {
        static uint32_t s_ctorCount = 0;
        static uint32_t s_dtorCount = 0;
        struct CtorTest { CtorTest(uint32_t x) { s_ctorCount += x; } ~CtorTest() { ++s_dtorCount; } };

        s_ctorCount = 0;
        s_dtorCount = 0;

        CtorTest* mem = alloc.New<CtorTest>(16);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, alignof(CtorTest)));
        HE_EXPECT_EQ(s_ctorCount, 16);
        HE_EXPECT_EQ(s_dtorCount, 0);

        alloc.Delete(mem);
        HE_EXPECT_EQ(s_ctorCount, 16);
        HE_EXPECT_EQ(s_dtorCount, 1);
    }

    // NewArray -> DeleteArray trivial
    {
        uint32_t* mem = alloc.NewArray<uint32_t>(16);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, alignof(uint32_t)));

        alloc.DeleteArray(mem);
    }

    // NewArray -> DeleteArray non-trivial
    {
        static uint32_t s_ctorCount = 0;
        static uint32_t s_dtorCount = 0;
        struct CtorTest { CtorTest(uint32_t x) { s_ctorCount += x; } ~CtorTest() { ++s_dtorCount; } };

        s_ctorCount = 0;
        s_dtorCount = 0;

        CtorTest* mem = alloc.NewArray<CtorTest>(16, 2);
        HE_EXPECT(mem);
        HE_EXPECT(IsAligned(mem, alignof(CtorTest)));
        HE_EXPECT_EQ(s_ctorCount, 32);
        HE_EXPECT_EQ(s_dtorCount, 0);

        alloc.DeleteArray(mem);
        HE_EXPECT_EQ(s_ctorCount, 32);
        HE_EXPECT_EQ(s_dtorCount, 16);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, allocator, Allocator)
{
    TestAllocator(Allocator::GetDefault());
    TestAllocator(Allocator::GetTemp());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, allocator, CrtAllocator)
{
    CrtAllocator a;
    TestAllocator(a);

    TestAllocator(CrtAllocator::Get());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, allocator, LinearPageAllocator)
{
    LinearPageAllocator a(1024);
    TestAllocator(a);

    // Allocate enough to cause multiple pages to be created
    void* alloc0 = a.Malloc(512);
    void* alloc1 = a.Malloc(512);
    void* alloc2 = a.Malloc(512);
    void* alloc3 = a.Malloc(512);
    void* alloc4 = a.Malloc(512);
    void* alloc5 = a.Malloc(512);
    void* alloc6 = a.Malloc(512);

    // Write to the allocations
    MemSet(alloc0, 0, 512);
    MemSet(alloc1, 1, 512);
    MemSet(alloc2, 2, 512);
    MemSet(alloc3, 3, 512);
    MemSet(alloc4, 4, 512);
    MemSet(alloc5, 5, 512);
    MemSet(alloc6, 6, 512);

    // Check that each has the right memory set (checking for overlap of blocks)
    uint8_t expectedBytes[512];
    MemSet(expectedBytes, 0, 512);
    HE_EXPECT_EQ_MEM(alloc0, expectedBytes, 512);
    MemSet(expectedBytes, 1, 512);
    HE_EXPECT_EQ_MEM(alloc1, expectedBytes, 512);
    MemSet(expectedBytes, 2, 512);
    HE_EXPECT_EQ_MEM(alloc2, expectedBytes, 512);
    MemSet(expectedBytes, 3, 512);
    HE_EXPECT_EQ_MEM(alloc3, expectedBytes, 512);
    MemSet(expectedBytes, 4, 512);
    HE_EXPECT_EQ_MEM(alloc4, expectedBytes, 512);
    MemSet(expectedBytes, 5, 512);
    HE_EXPECT_EQ_MEM(alloc5, expectedBytes, 512);
    MemSet(expectedBytes, 6, 512);
    HE_EXPECT_EQ_MEM(alloc6, expectedBytes, 512);

    // Finally, free it all up
    a.Free(alloc0);
    a.Free(alloc1);
    a.Free(alloc2);
    a.Free(alloc3);
    a.Free(alloc4);
    a.Free(alloc5);
    a.Free(alloc6);
}
