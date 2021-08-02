// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/test.h"
#include "he/core/utils.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, allocator, Test)
{
    CrtAllocator a;
    void* p = a.Malloc(16, 16);

    HE_EXPECT(p);
    HE_EXPECT(IsAligned(p, 16));

    void* p2 = a.Malloc(16, 8);

    HE_EXPECT(p2);
    HE_EXPECT(IsAligned(p2, 8));

    HE_EXPECT(p != p2);

    a.Free(p);
    a.Free(p2);
}
