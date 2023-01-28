// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/tracking_allocator.h"

#include "he/core/result_fmt.h"
#include "he/core/stack_trace.h"
#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tracking_allocator, Test)
{
    TrackingAllocator a(Allocator::GetDefault());
    TestAllocator(a);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tracking_allocator, ActiveAllocations)
{
    TrackingAllocator a(Allocator::GetDefault());

    HE_EXPECT_EQ(a.ActiveAllocationCount(), 0);
    void* p = a.Malloc(1024);
    HE_EXPECT_EQ(a.ActiveAllocationCount(), 1);

    a.ForEachActiveAllocation([&](size_t size, Span<const uintptr_t> frames)
    {
        HE_EXPECT_EQ(size, 1024);
        HE_EXPECT_GT(frames.Size(), 2);

        SymbolInfo info;
        Result r = GetSymbolInfo(frames[0], info);
        HE_EXPECT(r, r);
        HE_EXPECT_EQ(info.name, "_heTestClass_core_tracking_allocator_ActiveAllocations::TestBody");
    });

    HE_EXPECT_EQ(a.ActiveAllocationCount(), 1);
    a.Free(p);
    HE_EXPECT_EQ(a.ActiveAllocationCount(), 0);
}
