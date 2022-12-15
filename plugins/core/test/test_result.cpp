// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/string_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, result, Success)
{
    Result r;
    HE_EXPECT(r);

    String msg;
    r.ToString(msg);
    HE_EXPECT(!msg.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, result, InvalidParameter)
{
    Result r = Result::InvalidParameter;
    HE_EXPECT(!r);

    String msg;
    r.ToString(msg);
    HE_EXPECT(!msg.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, result, NotSupported)
{
    Result r = Result::NotSupported;
    HE_EXPECT(!r);

    String msg;
    r.ToString(msg);
    HE_EXPECT(!msg.IsEmpty());
}
