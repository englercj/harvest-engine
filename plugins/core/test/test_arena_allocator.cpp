// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/arena_allocator.h"

#include "he/core/memory_ops.h"
#include "he/core/random.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, arena_allocator, basic)
{
    ArenaAllocator a;
    TestAllocator(a);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, arena_allocator, multipage)
{
    ArenaAllocator a;

    static constexpr size_t AllocSize = 1024;

    // Allocate enough to cause multiple pages to be committed
    void* alloc0 = a.Malloc(AllocSize);
    void* alloc1 = a.Malloc(AllocSize);
    void* alloc2 = a.Malloc(AllocSize);
    void* alloc3 = a.Malloc(AllocSize);
    void* alloc4 = a.Malloc(AllocSize);
    void* alloc5 = a.Malloc(AllocSize);
    void* alloc6 = a.Malloc(AllocSize);
    HE_EXPECT_GT(ArenaAllocatorTestAttorney::GetCommittedPages(a), 1);

    // Write to the allocations
    MemSet(alloc0, 0, AllocSize);
    MemSet(alloc1, 1, AllocSize);
    MemSet(alloc2, 2, AllocSize);
    MemSet(alloc3, 3, AllocSize);
    MemSet(alloc4, 4, AllocSize);
    MemSet(alloc5, 5, AllocSize);
    MemSet(alloc6, 6, AllocSize);

    // Check that each has the right memory set (e.g.: no memory stomps from overlapping allocs)
    uint8_t expectedBytes[AllocSize];
    MemSet(expectedBytes, 0, AllocSize);
    HE_EXPECT_EQ_MEM(alloc0, expectedBytes, AllocSize);
    MemSet(expectedBytes, 1, AllocSize);
    HE_EXPECT_EQ_MEM(alloc1, expectedBytes, AllocSize);
    MemSet(expectedBytes, 2, AllocSize);
    HE_EXPECT_EQ_MEM(alloc2, expectedBytes, AllocSize);
    MemSet(expectedBytes, 3, AllocSize);
    HE_EXPECT_EQ_MEM(alloc3, expectedBytes, AllocSize);
    MemSet(expectedBytes, 4, AllocSize);
    HE_EXPECT_EQ_MEM(alloc4, expectedBytes, AllocSize);
    MemSet(expectedBytes, 5, AllocSize);
    HE_EXPECT_EQ_MEM(alloc5, expectedBytes, AllocSize);
    MemSet(expectedBytes, 6, AllocSize);
    HE_EXPECT_EQ_MEM(alloc6, expectedBytes, AllocSize);

    // Finally, free it all up
    a.Free(alloc0);
    a.Free(alloc1);
    a.Free(alloc2);
    a.Free(alloc3);
    a.Free(alloc4);
    a.Free(alloc5);
    a.Free(alloc6);
}
