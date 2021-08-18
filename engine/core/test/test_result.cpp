// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/allocator.h"
#include "he/core/string_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, result, Success)
{
    Result r;
    HE_EXPECT(r);

    String msg = r.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(msg, "success");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, result, InvalidArgument)
{
    Result r = Result::InvalidParameter;
    HE_EXPECT(!r);

    String msg = r.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(msg, "invalid argument");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, result, NotSupported)
{
    Result r = Result::NotSupported;
    HE_EXPECT(!r);

    String msg = r.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(msg, "not supported");
}
