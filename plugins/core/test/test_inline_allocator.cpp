// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/inline_allocator.h"

#include "he/core/memory_ops.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, inline_allocator, test)
{
    InlineAllocator<64> a(Allocator::GetDefault());
    TestAllocatorNoRealloc(a);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, inline_allocator, fallback)
{
    InlineAllocator<2048> a(Allocator::GetDefault());

    // Allocate enough to cause the fallback allocator to be used
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

    // Check that each has the right memory set (e.g.: no memory stomps from overlapping allocs)
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
