// Copyright Chad Engler

#include "he/core/error.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, error, ErrorHandler)
{
    ErrorHandlerFunc original = GetErrorHandler();

    auto handler = [](ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg) -> bool
    {
        HE_EXPECT_EQ(type, ErrorType::Expect);
        HE_EXPECT_EQ_STR(file, "test file");
        HE_EXPECT_EQ(line, 12345);
        HE_EXPECT_EQ_STR(funcName, "test func sig");
        HE_EXPECT_EQ_STR(expression, "test expr");
        HE_EXPECT_EQ_STR(msg, "test msg");
        return false;
    };

    {
        ScopedErrorHandler scope(handler);
        HE_EXPECT(GetErrorHandler() == handler);

        bool handleResult = HandleError(ErrorType::Expect, "test file", 12345, "test func sig", "test expr", "test msg");
        HE_EXPECT_EQ(handleResult, false);
    }

    HE_EXPECT(GetErrorHandler() == original);
}
