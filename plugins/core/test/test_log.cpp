// Copyright Chad Engler

#include "he/core/log.h"

#include "he/core/compiler.h"
#include "he/core/path.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log, LogLevel)
{
    // These values changing is potentially breaking, so checking them here.
    static_assert(HE_LOG_LEVEL_TRACE == 0);
    static_assert(HE_LOG_LEVEL_DEBUG == 1);
    static_assert(HE_LOG_LEVEL_INFO == 2);
    static_assert(HE_LOG_LEVEL_WARN == 3);
    static_assert(HE_LOG_LEVEL_ERROR == 4);

    static_assert(static_cast<uint8_t>(LogLevel::Trace) == HE_LOG_LEVEL_TRACE);
    static_assert(static_cast<uint8_t>(LogLevel::Debug) == HE_LOG_LEVEL_DEBUG);
    static_assert(static_cast<uint8_t>(LogLevel::Info) == HE_LOG_LEVEL_INFO);
    static_assert(static_cast<uint8_t>(LogLevel::Warn) == HE_LOG_LEVEL_WARN);
    static_assert(static_cast<uint8_t>(LogLevel::Error) == HE_LOG_LEVEL_ERROR);

    static_assert(IsSame<std::underlying_type_t<LogLevel>, uint8_t>);

    static_assert(HE_LOG_ENABLE_LEVEL >= HE_LOG_LEVEL_TRACE && HE_LOG_ENABLE_LEVEL <= HE_LOG_LEVEL_ERROR);
}

static void TestLogHandler(const void*, const LogSource& source, const KeyValue* kvs, uint32_t count)
{
    HE_EXPECT_EQ(source.level, LogLevel::Info);
#if !HE_INTERNAL_BUILD
    HE_EXPECT(source.line == 0);
    HE_EXPECT_EQ_STR(source.file, "");
#else
    HE_EXPECT(source.line == 64 || source.line == 65);
    HE_EXPECT_EQ(GetBaseName(source.file), "test_log.cpp");
#endif

#if HE_COMPILER_MSVC
    HE_EXPECT_EQ_STR(source.funcName, "void __cdecl _heTestClass_core_log_AddLogSink_RemoveLogSink::TestBody(void)");
#elif HE_COMPILER_GCC
    HE_EXPECT_EQ_STR(source.funcName, "virtual void _heTestClass_core_log_AddLogSink_RemoveLogSink::TestBody()");
#else
    #error "No func sig test for this compiler"
#endif

    HE_EXPECT_EQ_STR(source.category, "log_test");

    HE_EXPECT_EQ(count, 1);
    HE_EXPECT_EQ_STR(kvs[0].Key(), HE_MSG_KEY);
    HE_EXPECT_EQ(kvs[0].Kind(), KeyValue::ValueKind::String);
    HE_EXPECT_EQ(kvs[0].String(), "testing");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log, AddLogSink_RemoveLogSink)
{
    AddLogSink(TestLogHandler);

    HE_LOG_INFO(log_test, HE_MSG("testing"));
    HE_LOGF_INFO(log_test, "testing");

    RemoveLogSink(TestLogHandler);
}
