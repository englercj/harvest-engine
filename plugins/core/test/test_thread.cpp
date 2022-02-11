// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/result_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, GetCurrentThreadHandle)
{
    ThreadHandle self = GetCurrentThreadHandle();
    HE_EXPECT_NE(self, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, SetThreadAffinity)
{
    ThreadHandle self = GetCurrentThreadHandle();
    Result r = SetThreadAffinity(self, 0xffffffffffffffff);
    HE_EXPECT(r, r);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, SetCurrentThreadName)
{
    SetCurrentThreadName("HE Test Thread");
}
