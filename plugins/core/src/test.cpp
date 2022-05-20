// Copyright Chad Engler

#include "he/core/test.h"

#include "he/core/appender.h"
#include "he/core/clock.h"
#include "he/core/error.h"
#include "he/core/string.h"
#include "he/core/vector.h"

#include "fmt/core.h"

#include <algorithm>
#include <atomic>

namespace he
{
namespace internal
{
    std::atomic<uint32_t> g_totalTestRuns{ 0 };
    std::atomic<uint32_t> g_totalTestExpects{ 0 };
    std::atomic<uint32_t> g_totalTestFailures{ 0 };
}

    const TestInfo TestFixture::EmptyTestInfo{};

    struct _TestRunner
    {
        static _TestRunner& Get()
        {
            static _TestRunner s_runner{};
            return s_runner;
        }

        Vector<TestFixture*> tests{};
    };

    TestFixture::TestFixture()
    {
        // TODO: Can probably make this a linked list andavoid the allocation at static construction that happens here.
        _TestRunner::Get().tests.PushBack(this);
    }

    void TestFixture::Run()
    {
        TestBody();
    }

    static void SortTests()
    {
        _TestRunner& runner = _TestRunner::Get();

        std::sort(runner.tests.begin(), runner.tests.end(), [](const TestFixture* a, const TestFixture* b)
        {
            const TestInfo& infoA = a->GetTestInfo();
            const TestInfo& infoB = b->GetTestInfo();

            const int32_t moduleNameCmp = String::CompareI(infoA.moduleName, infoB.moduleName);
            if (moduleNameCmp != 0)
                return moduleNameCmp < 0;

            const int32_t suiteNameCmp = String::CompareI(infoA.suiteName, infoB.suiteName);
            if (suiteNameCmp != 0)
                return suiteNameCmp < 0;

            const int32_t testNameCmp = String::CompareI(infoA.testName, infoB.testName);
            if (testNameCmp != 0)
                return testNameCmp < 0;

            return false;
        });
    }

    static bool TestLibErrorHandler(void*, const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        LogSource logSource;
        logSource.level = LogLevel::Error;
        logSource.line = source.line;
        logSource.file = source.file;
        logSource.funcName = source.funcName;
        logSource.category = "he_test";

        Log(logSource, kvs, count);

        ErrorKind kind = kvs[0].GetEnum<ErrorKind>();
        switch (kind)
        {
            case ErrorKind::Assert:
            case ErrorKind::Except:
                std::abort();
                break;
            case ErrorKind::Expect:
            case ErrorKind::Verify:
                break;
        }

        return true;
    }

    uint32_t RunAllTests(const char* filter)
    {
        SortTests();

        _TestRunner& runner = _TestRunner::Get();

        String testFqn;

        for (TestFixture* fixture : runner.tests)
        {
            const TestInfo& info = fixture->GetTestInfo();

            if (!String::IsEmpty(filter))
            {
                testFqn.Clear();
                fmt::format_to(Appender(testFqn), "{}:{}:{}", info.moduleName, info.suiteName, info.testName);
                if (String::Find(testFqn.Data(), filter) == nullptr)
                    continue;
            }

            ++internal::g_totalTestRuns;

            HE_LOGF_INFO(he_test, "Running {}:{}:{}", info.moduleName, info.suiteName, info.testName);

            MonotonicTime start;
            MonotonicTime end;
            {
                ScopedErrorHandler errorGuard(TestLibErrorHandler);

                start = MonotonicClock::Now();

                fixture->Before();
                fixture->Run();
                fixture->After();

                end = MonotonicClock::Now();
            }

            HE_LOG_INFO(he_test,
                HE_KV(test_event_kind, TestEventKind::TestTiming),
                HE_KV(test_time_ns, (end - start).val));
        }

        HE_LOGF_INFO(he_test, "Ran {} tests with {} expectations. {} tests failed.",
            internal::g_totalTestRuns.load(), internal::g_totalTestExpects.load(), internal::g_totalTestFailures.load());

        return internal::g_totalTestFailures.load();
    }

    template <>
    const char* AsString(TestEventKind x)
    {
        switch (x)
        {
            case TestEventKind::TestTiming: return "TestTiming";
        }

        return "<unknown>";
    }
}
