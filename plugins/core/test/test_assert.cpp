// Copyright Chad Engler

#include "he/core/assert.h"

#include "he/core/error.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, assert, ASSERT)
{
    auto handler = [](void* userData, const ErrorSource& source, const LogKV* kvs, uint32_t count) -> bool
    {
        HE_EXPECT_EQ(source.type, ErrorType::Assert);
        HE_EXPECT_EQ(source.line, 27);
        HE_EXPECT_EQ_STR(source.expression, "false");

        HE_EXPECT_EQ(count, 1);
        HE_EXPECT_EQ_STR(kvs[0].key, HE_LOG_MESSAGE_KEY);
        HE_EXPECT_EQ(kvs[0].kind, LogKV::Kind::String);
        HE_EXPECT_EQ(kvs[0].GetString(), "testing 10");
        return false;
    };

    ErrorHandlerFunc oldHandler = GetErrorHandler();
    SetErrorHandler(handler);

    HE_ASSERT(false, HE_MSG("testing {}", 10));

    SetErrorHandler(oldHandler);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, assert, VERIFY)
{
    auto handler = [](void* userData, const ErrorSource& source, const LogKV* kvs, uint32_t count) -> bool
    {
        HE_EXPECT_EQ(source.type, ErrorType::Verify);
        HE_EXPECT_EQ(source.line, 49);
        HE_EXPECT_EQ_STR(source.expression, "false");

        HE_EXPECT_EQ(count, 1);
        HE_EXPECT_EQ_STR(kvs[0].key, HE_LOG_MESSAGE_KEY);
        HE_EXPECT_EQ(kvs[0].kind, LogKV::Kind::String);
        HE_EXPECT_EQ(kvs[0].GetString(), "testing 20");
        return false;
    };

    ErrorHandlerFunc oldHandler = GetErrorHandler();
    SetErrorHandler(handler);

    HE_EXPECT(!HE_VERIFY(false, HE_MSG("testing {}", 20)));

    SetErrorHandler(oldHandler);
}
