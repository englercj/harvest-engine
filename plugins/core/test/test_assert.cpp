// Copyright Chad Engler

#include "he/core/assert.h"

#include "he/core/error.h"
#include "he/core/path.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, assert, ASSERT)
{
    auto handler = [](void*, const ErrorSource& source, const KeyValue* kvs, uint32_t count) -> bool
    {
        HE_EXPECT_EQ(source.line, 43);
        HE_EXPECT_EQ(GetBaseName(source.file), "test_assert.cpp");
    #if HE_COMPILER_MSVC
        HE_EXPECT_EQ_STR(source.funcName, "void __cdecl _heTestClass_core_assert_ASSERT::TestBody(void)");
    #elif HE_COMPILER_GCC
        HE_EXPECT_EQ_STR(source.funcName, "virtual void _heTestClass_core_assert_ASSERT::TestBody()");
    #endif

        HE_EXPECT_EQ(count, 3);

        HE_EXPECT_EQ_STR(kvs[0].Key(), "error_kind");
        HE_EXPECT_EQ(kvs[0].Kind(), KeyValue::ValueKind::Enum);
        HE_EXPECT_EQ(kvs[0].GetEnum<ErrorKind>(), ErrorKind::Assert);

        HE_EXPECT_EQ_STR(kvs[1].Key(), "error_expr");
        HE_EXPECT_EQ(kvs[1].Kind(), KeyValue::ValueKind::String);
        HE_EXPECT_EQ(kvs[1].GetString(), "false");

        HE_EXPECT_EQ_STR(kvs[2].Key(), HE_MSG_KEY);
        HE_EXPECT_EQ(kvs[2].Kind(), KeyValue::ValueKind::String);
        HE_EXPECT_EQ(kvs[2].GetString(), "testing 10");

        return false;
    };

    ScopedErrorHandler errorGuard(handler);

    HE_ASSERT(false, HE_MSG("testing {}", 10));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, assert, VERIFY)
{
    auto handler = [](void*, const ErrorSource& source, const KeyValue* kvs, uint32_t count) -> bool
    {
        HE_EXPECT_EQ(source.line, 79);
        HE_EXPECT_EQ(GetBaseName(source.file), "test_assert.cpp");
    #if HE_COMPILER_MSVC
        HE_EXPECT_EQ_STR(source.funcName, "void __cdecl _heTestClass_core_assert_VERIFY::TestBody(void)");
    #elif HE_COMPILER_GCC
        HE_EXPECT_EQ_STR(source.funcName, "virtual void _heTestClass_core_assert_VERIFY::TestBody()");
    #else
        #error "Need to add test for: " HE_FUNC_SIG
    #endif

        HE_EXPECT_EQ(count, 3);
        HE_EXPECT_EQ_STR(kvs[0].Key(), "error_kind");
        HE_EXPECT_EQ(kvs[0].Kind(), KeyValue::ValueKind::Enum);
        HE_EXPECT_EQ(kvs[0].GetEnum<ErrorKind>(), ErrorKind::Verify);

        HE_EXPECT_EQ_STR(kvs[1].Key(), "error_expr");
        HE_EXPECT_EQ(kvs[1].Kind(), KeyValue::ValueKind::String);
        HE_EXPECT_EQ(kvs[1].GetString(), "false");

        HE_EXPECT_EQ_STR(kvs[2].Key(), HE_MSG_KEY);
        HE_EXPECT_EQ(kvs[2].Kind(), KeyValue::ValueKind::String);
        HE_EXPECT_EQ(kvs[2].GetString(), "testing 20");

        return false;
    };

    ScopedErrorHandler errorGuard(handler);

    HE_EXPECT(!HE_VERIFY(false, HE_MSG("testing {}", 20)));
}
