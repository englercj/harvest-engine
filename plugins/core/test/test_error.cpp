// Copyright Chad Engler

#include "he/core/error.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, error, ErrorHandler)
{
    void* originalUserData = nullptr;
    ErrorHandlerFunc originalHandler = GetErrorHandler(originalUserData);

    auto handler = [](void*, const ErrorSource& source, const KeyValue* kvs, uint32_t count) -> bool
    {
        HE_EXPECT_EQ(source.line, 12345);
        HE_EXPECT_EQ_STR(source.file, "test file name");
        HE_EXPECT_EQ_STR(source.funcName, "test func sig");

        HE_EXPECT_EQ(count, 2);

        HE_EXPECT_EQ_STR(kvs[0].Key(), "a");
        HE_EXPECT_EQ(kvs[0].Kind(), KeyValue::ValueKind::String);
        HE_EXPECT_EQ(kvs[0].GetString(), "a");

        HE_EXPECT_EQ_STR(kvs[1].Key(), "b");
        HE_EXPECT_EQ(kvs[1].Kind(), KeyValue::ValueKind::String);
        HE_EXPECT_EQ(kvs[1].GetString(), "b");

        return false;
    };

    bool handleResult = true;
    {
        void* testUserData = reinterpret_cast<void*>(0x123456789);
        ScopedErrorHandler scope(handler, testUserData);

        void* errorUserData = nullptr;
        ErrorHandlerFunc errorHandler = GetErrorHandler(errorUserData);
        HE_EXPECT_EQ_PTR(errorHandler, static_cast<ErrorHandlerFunc>(handler));
        HE_EXPECT_EQ_PTR(errorUserData, testUserData);

        ErrorSource source;
        source.line = 12345;
        source.file = "test file name";
        source.funcName = "test func sig";

        const KeyValue kvs[]{ HE_KV(a, "a"), HE_KV(b, "b") };

        handleResult = HandleError(source, kvs, HE_LENGTH_OF(kvs));
    }
    HE_EXPECT_EQ(handleResult, false);

    void* errorUserData = nullptr;
    ErrorHandlerFunc errorHandler = GetErrorHandler(errorUserData);
    HE_EXPECT_EQ_PTR(errorHandler, originalHandler);
    HE_EXPECT_EQ_PTR(errorUserData, originalUserData);
}
