// Copyright Chad Engler

#include "he/core/assert.h"

#include "he/core/error.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, assert, ASSERT)
{
    auto handler = [](ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg) -> bool
    {
        HE_UNUSED(file, funcName);

        HE_EXPECT_EQ(type, ErrorType::Assert);
        HE_EXPECT_EQ(line, 27);
        HE_EXPECT_EQ_STR(expression, "false");
        HE_EXPECT_EQ_STR(msg, "testing 10");
        return false;
    };

    ErrorHandlerFunc oldHandler = GetErrorHandler();
    SetErrorHandler(handler);

    HE_ASSERT(false, "testing {}", 10);

    SetErrorHandler(oldHandler);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, assert, VERIFY)
{
    auto handler = [](ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg) -> bool
    {
        HE_UNUSED(file, funcName);

        HE_EXPECT_EQ(type, ErrorType::Verify);
        HE_EXPECT_EQ(line, 49);
        HE_EXPECT_EQ_STR(expression, "false");
        HE_EXPECT_EQ_STR(msg, "testing 20");
        return false;
    };

    ErrorHandlerFunc oldHandler = GetErrorHandler();
    SetErrorHandler(handler);

    HE_EXPECT(!HE_VERIFY(false, "testing {}", 20));

    SetErrorHandler(oldHandler);
}
