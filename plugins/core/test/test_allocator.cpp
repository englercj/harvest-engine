// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/allocator.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, allocator, Allocator_GetDefault)
{
    TestAllocator(Allocator::GetDefault());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, allocator, CrtAllocator)
{
    CrtAllocator a;
    TestAllocator(a);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, allocator, CrtAllocator_Get)
{
    TestAllocator(CrtAllocator::Get());
}
